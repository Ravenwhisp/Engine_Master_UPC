#include "Globals.h"
#include "ShadowCasterCullingPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "RenderContext.h"
#include "RingBuffer.h"

#include "MeshRenderer.h"
#include "BasicMesh.h"
#include "MeshAsset.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GameObject.h"
#include "Transform.h"
#include "Skin.h"

#include "ShadowFrustumComputePass.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include "PlatformHelpers.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <chrono>

namespace
{
    void splitGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS address, uint32_t& low, uint32_t& high)
    {
        low = static_cast<uint32_t>(address & 0xffffffffull);
        high = static_cast<uint32_t>(address >> 32);
    }

    void storeMatrix(const Matrix& matrix, float output[16])
    {
        static_assert(sizeof(Matrix) == sizeof(float) * 16);
        std::memcpy(output, &matrix, sizeof(float) * 16);
    }

    uint64_t getNextShadowCasterRegistryId()
    {
        static uint64_t nextRegistryId = 1;
        return nextRegistryId++;
    }
}

ShadowCasterCullingPass::ShadowCasterCullingPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass)
    : m_device(device)
    , m_shadowFrustumComputePass(shadowFrustumComputePass)
{
    m_registryId = getNextShadowCasterRegistryId();

    createRootSignature();
    createPipelineState();
    createCounterResources();
}

void ShadowCasterCullingPass::prepare(const RenderContext& ctx)
{
    ModuleD3D12* d3d12 = app->getModuleD3D12();

    if (d3d12 == nullptr)
    {
        return;
    }

    ensureRegistryBootstrap();

    const uint32_t frameIndex = d3d12->getCurrentFrameIndex();
    const uint64_t fenceValue = d3d12->getCurrentFrame();

    if (m_candidateFrameIndex == frameIndex && m_candidateFenceValue == fenceValue)
    {
        return;
    }

    using Clock = std::chrono::steady_clock;

    m_preparationStats = {};

    const auto totalStart = Clock::now();

    processDirtyQueue(ctx);
    preparePersistentCandidateBuffer(ctx);
    flushRetiredPersistentCandidateSlots(ctx);
    refreshSkinnedPersistentCandidates(ctx);

    m_candidateBufferAddress = 0;
    m_candidateCount = 0;

    const bool fullUploadReady =
        m_pendingPersistentUploadResource != nullptr &&
        m_pendingPersistentUploadSize > 0;

    const bool persistentReady =
        m_persistentCandidateBuffer != nullptr &&
        (!m_persistentCandidateNeedsFullUpload || fullUploadReady);

    if (persistentReady && m_candidateSlotHighWaterMark > 0)
    {
        m_candidateBufferAddress = m_persistentCandidateBuffer->GetGPUVirtualAddress();
        m_candidateCount = m_candidateSlotHighWaterMark;

        const auto capacityStart = Clock::now();

        ensureVisibilityMaskCapacity(m_candidateCount);
        ensureIndirectCommandCapacity(m_candidateCount);

        const auto capacityEnd = Clock::now();

        m_preparationStats.capacityMs = std::chrono::duration<float, std::milli>(capacityEnd - capacityStart).count();
    }

    m_preparationStats.candidateCount = m_candidateCount;
    m_preparationStats.skinnedRenderers = static_cast<uint32_t>(m_skinnedHandles.size());
    m_preparationStats.skinnedCandidateCount = m_pendingPersistentUpdatedSlotCount;

    if (m_pendingPersistentUploadSize > 0)
    {
        m_preparationStats.uploadBytes = m_pendingPersistentUploadSize;
    }
    else
    {
        m_preparationStats.uploadBytes = m_pendingPersistentUpdatedBytes;
    }

    const auto totalEnd = Clock::now();

    m_preparationStats.totalMs = std::chrono::duration<float, std::milli>(totalEnd - totalStart).count();

    m_candidateFrameIndex = frameIndex;
    m_candidateFenceValue = fenceValue;

    ++m_preparationProfileLogCounter;

    if (m_preparationProfileLogCounter >= 120)
    {
        DEBUG_LOG(
            "[Shadow Persistent Prep] Total %.3f ms | Capacity %.3f ms | Slots %u | Live %u | Skinned %u | Pending upload %.2f KiB",
            m_preparationStats.totalMs,
            m_preparationStats.capacityMs,
            m_candidateCount,
            m_liveCandidateSlotCount,
            static_cast<uint32_t>(m_skinnedHandles.size()),
            static_cast<double>(m_preparationStats.uploadBytes) / 1024.0);

        m_preparationProfileLogCounter = 0;
    }
}

void ShadowCasterCullingPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "ShadowCasterCullingPass");

    uploadPendingPersistentCandidates(commandList);

    if (commandList == nullptr ||
        m_shadowFrustumComputePass == nullptr ||
        !m_shadowFrustumComputePass->hasValidResult() ||
        m_candidateBufferAddress == 0 ||
        m_candidateCount == 0 ||
        !m_visibilityMaskBuffer ||
        !m_cascadeCountBuffer)
    {
        END_EVENT(commandList);
        return;
    }

    const D3D12_GPU_VIRTUAL_ADDRESS shadowDataAddress = m_shadowFrustumComputePass->getShadowDataBufferAddress();

    if (shadowDataAddress == 0)
    {
        END_EVENT(commandList);
        return;
    }

    transitionCascadeCountBuffer(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

    commandList->CopyBufferRegion(
        m_cascadeCountBuffer.Get(),
        0,
        m_zeroCountUploadBuffer.Get(),
        0,
        sizeof(uint32_t) * MAX_SHADOW_CASCADES);

    transitionCascadeCountBuffer(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    transitionVisibilityMaskBuffer(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    for (uint32_t cascadeIndex = 0; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex)
    {
        transitionIndirectCommandBuffer(commandList, cascadeIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetComputeRootSignature(m_rootSignature.Get());

    commandList->SetComputeRootShaderResourceView(0, m_candidateBufferAddress);
    commandList->SetComputeRootUnorderedAccessView(1, m_visibilityMaskBuffer->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(2, m_cascadeCountBuffer->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(3, shadowDataAddress);
    commandList->SetComputeRoot32BitConstant(4, m_candidateCount, 0);
    commandList->SetComputeRootUnorderedAccessView(5, m_indirectCommandBuffers[0]->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(6, m_indirectCommandBuffers[1]->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(7, m_indirectCommandBuffers[2]->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(8, m_indirectCommandBuffers[3]->GetGPUVirtualAddress());

    const uint32_t groupCount = (m_candidateCount + 63u) / 64u;
    commandList->Dispatch(groupCount, 1, 1);

    const CD3DX12_RESOURCE_BARRIER barriers[] =
    {
        CD3DX12_RESOURCE_BARRIER::UAV(m_visibilityMaskBuffer.Get()),
        CD3DX12_RESOURCE_BARRIER::UAV(m_cascadeCountBuffer.Get()),
        CD3DX12_RESOURCE_BARRIER::UAV(m_indirectCommandBuffers[0].Get()),
        CD3DX12_RESOURCE_BARRIER::UAV(m_indirectCommandBuffers[1].Get()),
        CD3DX12_RESOURCE_BARRIER::UAV(m_indirectCommandBuffers[2].Get()),
        CD3DX12_RESOURCE_BARRIER::UAV(m_indirectCommandBuffers[3].Get())
    };

    commandList->ResourceBarrier(_countof(barriers), barriers);

    for (uint32_t cascadeIndex = 0; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex)
    {
        transitionIndirectCommandBuffer(commandList, cascadeIndex, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    transitionCascadeCountBuffer(commandList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    END_EVENT(commandList);
}

void ShadowCasterCullingPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParameters[9] = {};

    // t0: ShadowCasterCandidateGPU[]
    rootParameters[0].InitAsShaderResourceView(0, 0);

    // u0: visibility masks
    rootParameters[1].InitAsUnorderedAccessView(0, 0);

    // u1: per-cascade visible counters
    rootParameters[2].InitAsUnorderedAccessView(1, 0);

    // b0: ShadowDataCB
    rootParameters[3].InitAsConstantBufferView(0, 0);

    // b1: candidateCount
    rootParameters[4].InitAsConstants(1, 1, 0);

    // u2-u5: compacted indirect commands
    rootParameters[5].InitAsUnorderedAccessView(2, 0);
    rootParameters[6].InitAsUnorderedAccessView(3, 0);
    rootParameters[7].InitAsUnorderedAccessView(4, 0);
    rootParameters[8].InitAsUnorderedAccessView(5, 0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void ShadowCasterCullingPass::createPipelineState()
{
    ComPtr<ID3DBlob> computeShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"ShadowCasterCullingComputeShader.cso", &computeShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

    const HRESULT hr = m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

    if (FAILED(hr))
    {
        char buffer[128];
        sprintf_s(buffer, "[ShadowCasterCulling] CreateComputePipelineState failed: 0x%08X\n", static_cast<unsigned int>(hr));
        OutputDebugStringA(buffer);

        ThrowIfFailed(hr);
    }
}

void ShadowCasterCullingPass::createCounterResources()
{
    constexpr size_t COUNTER_BUFFER_SIZE = sizeof(uint32_t) * MAX_SHADOW_CASCADES;

    m_cascadeCountBuffer = app->getModuleResources()->createDefaultBuffer(
        COUNTER_BUFFER_SIZE,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_DEST,
        "ShadowCascadeVisibleCounts");

    m_zeroCountUploadBuffer = app->getModuleResources()->createUploadBuffer(COUNTER_BUFFER_SIZE);

    uint32_t zeros[MAX_SHADOW_CASCADES] = {};

    void* mapped = nullptr;
    CD3DX12_RANGE readRange(0, 0);

    DXCall(m_zeroCountUploadBuffer->Map(0, &readRange, &mapped));
    std::memcpy(mapped, zeros, COUNTER_BUFFER_SIZE);
    m_zeroCountUploadBuffer->Unmap(0, nullptr);

    m_cascadeCountBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
}

void ShadowCasterCullingPass::ensureRegistryBootstrap()
{
    ModuleScene* moduleScene = app->getModuleScene();

    if (moduleScene == nullptr)
    {
        return;
    }

    Scene* scene = moduleScene->getScene();

    if (scene != m_registryScene)
    {
        resetRegistry();
        m_registryScene = scene;
    }

    if (scene == nullptr || m_registryInitialized)
    {
        return;
    }

    const std::vector<MeshRenderer*>& renderers = moduleScene->getMeshRenderers();

    if (renderers.empty())
    {
        return;
    }

    initializeRegistry();
    m_registryInitialized = true;
}

void ShadowCasterCullingPass::resetRegistry()
{
    m_registryEntries.clear();
    m_candidateSlots.clear();
    m_rendererHandles.clear();
    m_freeRegistryEntries.clear();
    m_freeCandidateSlots.clear();
    m_dirtyHandles.clear();
    m_skinnedHandles.clear();
    m_retiredCandidateSlots.clear();

    m_candidateSlotHighWaterMark = 0;
    m_liveCandidateSlotCount = 0;
    m_registryInitialized = false;

    m_registryId = getNextShadowCasterRegistryId();

    m_persistentCandidateNeedsFullUpload = true;

    m_pendingPersistentUploadResource = nullptr;
    m_pendingPersistentUploadOffset = 0;
    m_pendingPersistentUploadSize = 0;
    m_pendingPersistentCandidateCount = 0;
    m_pendingPersistentPopulatedCount = 0;
    m_pendingPersistentCandidateCopies.clear();
    m_pendingPersistentUpdatedSlotCount = 0;
    m_pendingPersistentUpdatedBytes = 0;
    m_persistentPartialUploadLogCounter = 0;
}

void ShadowCasterCullingPass::initializeRegistry()
{
    ModuleScene* moduleScene = app->getModuleScene();

    if (moduleScene == nullptr)
    {
        return;
    }

    const std::vector<MeshRenderer*>& renderers = moduleScene->getMeshRenderers();

    for (MeshRenderer* renderer : renderers)
    {
        if (renderer == nullptr)
        {
            continue;
        }

        registerRenderer(renderer);
    }

#ifdef _DEBUG
    uint32_t aliveRendererCount = 0;
    uint32_t assignedSlotCount = 0;
    std::vector<bool> seenSlots(m_candidateSlotHighWaterMark, false);

    for (uint32_t entryIndex = 0; entryIndex < m_registryEntries.size(); ++entryIndex)
    {
        const ShadowCasterRegistryEntry& entry = m_registryEntries[entryIndex];

        if (!entry.alive)
        {
            continue;
        }

        ++aliveRendererCount;

        assert(entry.renderer != nullptr);
        assert(entry.dirtyQueued);

        for (uint32_t slotIndex : entry.candidateSlots)
        {
            assert(slotIndex < m_candidateSlotHighWaterMark);
            assert(slotIndex < m_candidateSlots.size());
            assert(!seenSlots[slotIndex]);

            const ShadowCandidateSlot& slot = m_candidateSlots[slotIndex];

            assert(slot.alive);
            assert(slot.ownerEntryIndex == entryIndex);

            seenSlots[slotIndex] = true;
            ++assignedSlotCount;
        }
    }

    assert(aliveRendererCount == m_rendererHandles.size());
    assert(assignedSlotCount == m_liveCandidateSlotCount);
#endif

    DEBUG_LOG(
        "[Shadow Registry] Bootstrap | Renderers: %zu | Potential draws: %u | Skinned: %zu | High-water: %u",
        m_rendererHandles.size(),
        m_liveCandidateSlotCount,
        m_skinnedHandles.size(),
        m_candidateSlotHighWaterMark);
}

void ShadowCasterCullingPass::processDirtyQueue(const RenderContext& ctx)
{
    if (m_dirtyHandles.empty())
    {
        return;
    }

    std::vector<ShadowCasterHandle> pendingHandles;
    pendingHandles.swap(m_dirtyHandles);

    std::vector<uint32_t> dirtySlots;

    for (const ShadowCasterHandle& handle : pendingHandles)
    {
        if (!isRegistryHandleAlive(handle))
        {
            continue;
        }

        ShadowCasterRegistryEntry& entry = m_registryEntries[handle.index];

        if (entry.renderer == nullptr)
        {
            continue;
        }

        entry.dirtyQueued = false;

        const uint64_t revisionToProcess = entry.renderer->getShadowCandidateRevision();

        for (uint32_t slotIndex : entry.candidateSlots)
        {
            dirtySlots.push_back(slotIndex);
        }

        reconcileCandidateSlots(handle.index, entry);
        reconcileSkinnedMembership(handle, entry);

        for (uint32_t slotIndex : entry.candidateSlots)
        {
            dirtySlots.push_back(slotIndex);
        }

        entry.lastProcessedRevision = revisionToProcess;
    }

    if (dirtySlots.empty())
    {
        return;
    }

    std::sort(dirtySlots.begin(), dirtySlots.end());
    dirtySlots.erase(std::unique(dirtySlots.begin(), dirtySlots.end()), dirtySlots.end());

    if (!m_persistentCandidateBuffer || m_candidateSlotHighWaterMark > m_persistentCandidateCapacity)
    {
        m_persistentCandidateNeedsFullUpload = true;
        return;
    }

    if (m_persistentCandidateNeedsFullUpload)
    {
        return;
    }

    queuePersistentCandidateUpdates(dirtySlots, ctx);
}

void ShadowCasterCullingPass::queuePersistentCandidateUpdates(const std::vector<uint32_t>& slotIndices, const RenderContext& ctx)
{
    if (slotIndices.empty() || ctx.ringBuffer == nullptr || !m_persistentCandidateBuffer)
    {
        return;
    }

    std::vector<ShadowCasterCandidateGPU> candidates(slotIndices.size());

    for (size_t i = 0; i < slotIndices.size(); ++i)
    {
        buildPersistentCandidate(slotIndices[i], candidates[i]);
    }

    const size_t uploadSize = candidates.size() * sizeof(ShadowCasterCandidateGPU);
    const RingBufferAllocation allocation = ctx.ringBuffer->allocateWithInfo(candidates.data(), uploadSize);

    if (!allocation.isValid())
    {
        DEBUG_WARN("[Shadow Persistent] Partial upload allocation failed. Falling back to full upload.");
        m_persistentCandidateNeedsFullUpload = true;
        m_pendingPersistentCandidateCopies.clear();
        return;
    }

    m_pendingPersistentCandidateCopies.reserve(m_pendingPersistentCandidateCopies.size() + slotIndices.size());

    for (size_t i = 0; i < slotIndices.size(); ++i)
    {
        const uint32_t slotIndex = slotIndices[i];

        if (slotIndex >= m_persistentCandidateCapacity)
        {
            m_persistentCandidateNeedsFullUpload = true;
            m_pendingPersistentCandidateCopies.clear();
            return;
        }

        PendingPersistentCandidateCopy copy{};
        copy.sourceResource = allocation.resource;
        copy.sourceOffset = static_cast<uint64_t>(allocation.offset) + i * sizeof(ShadowCasterCandidateGPU);
        copy.destinationOffset = static_cast<uint64_t>(slotIndex) * sizeof(ShadowCasterCandidateGPU);
        copy.size = sizeof(ShadowCasterCandidateGPU);

        m_pendingPersistentCandidateCopies.push_back(copy);
    }

    m_pendingPersistentUpdatedSlotCount += static_cast<uint32_t>(slotIndices.size());
    m_pendingPersistentUpdatedBytes += uploadSize;
}

void ShadowCasterCullingPass::refreshSkinnedPersistentCandidates(const RenderContext& ctx)
{
    if (!m_registryInitialized ||
        m_persistentCandidateNeedsFullUpload ||
        !m_persistentCandidateBuffer ||
        ctx.ringBuffer == nullptr ||
        m_skinnedHandles.empty())
    {
        return;
    }

    std::vector<uint32_t> skinnedSlots;

    for (const ShadowCasterHandle& handle : m_skinnedHandles)
    {
        if (!isRegistryHandleAlive(handle))
        {
            continue;
        }

        const ShadowCasterRegistryEntry& entry = m_registryEntries[handle.index];

        if (!entry.alive || !entry.skinned || entry.renderer == nullptr)
        {
            continue;
        }

        for (uint32_t slotIndex : entry.candidateSlots)
        {
            skinnedSlots.push_back(slotIndex);
        }
    }

    if (skinnedSlots.empty())
    {
        return;
    }

    std::sort(skinnedSlots.begin(), skinnedSlots.end());
    skinnedSlots.erase(std::unique(skinnedSlots.begin(), skinnedSlots.end()), skinnedSlots.end());

    queuePersistentCandidateUpdates(skinnedSlots, ctx);
}

void ShadowCasterCullingPass::flushRetiredPersistentCandidateSlots(const RenderContext& ctx)
{
    if (m_retiredCandidateSlots.empty())
    {
        return;
    }

    std::sort(m_retiredCandidateSlots.begin(), m_retiredCandidateSlots.end());
    m_retiredCandidateSlots.erase(std::unique(m_retiredCandidateSlots.begin(), m_retiredCandidateSlots.end()), m_retiredCandidateSlots.end());

    if (!m_persistentCandidateBuffer ||
        m_persistentCandidateNeedsFullUpload ||
        m_candidateSlotHighWaterMark > m_persistentCandidateCapacity)
    {
        m_retiredCandidateSlots.clear();
        return;
    }

    queuePersistentCandidateUpdates(m_retiredCandidateSlots, ctx);
    m_retiredCandidateSlots.clear();
}
ShadowCasterHandle ShadowCasterCullingPass::registerRenderer(MeshRenderer * renderer)
{
    if (renderer == nullptr)
    {
        return {};
    }

    const auto existing = m_rendererHandles.find(renderer);

    if (existing != m_rendererHandles.end() && isRegistryHandleAlive(existing->second))
    {
        renderer->setShadowCasterHandle(existing->second);
        return existing->second;
    }

    uint32_t entryIndex = UINT32_MAX;

    if (!m_freeRegistryEntries.empty())
    {
        entryIndex = m_freeRegistryEntries.back();
        m_freeRegistryEntries.pop_back();
    }
    else
    {
        entryIndex = static_cast<uint32_t>(m_registryEntries.size());
        m_registryEntries.emplace_back();
    }

    ShadowCasterRegistryEntry& entry = m_registryEntries[entryIndex];

    entry.renderer = renderer;
    entry.alive = true;
    entry.dirtyQueued = true;
    entry.skinned = false;
    entry.lastProcessedRevision = 0;
    entry.skinnedListIndex = UINT32_MAX;
    entry.candidateSlots.clear();

    ShadowCasterHandle handle{};
    handle.registryId = m_registryId;
    handle.index = entryIndex;
    handle.generation = entry.generation;

    m_rendererHandles[renderer] = handle;
    m_dirtyHandles.push_back(handle);

    reconcileCandidateSlots(entryIndex, entry);
    reconcileSkinnedMembership(handle, entry);

    renderer->setShadowCasterHandle(handle);

    return handle;
}

void ShadowCasterCullingPass::unregisterRenderer(MeshRenderer* renderer, const ShadowCasterHandle& handle)
{
    if (renderer == nullptr || !isRegistryHandleAlive(handle))
    {
        return;
    }

    ShadowCasterRegistryEntry& entry = m_registryEntries[handle.index];

    if (entry.renderer != renderer)
    {
        return;
    }

    removeSkinnedHandle(entry);
    releaseCandidateSlots(entry);

    const auto rendererIt = m_rendererHandles.find(renderer);

    if (rendererIt != m_rendererHandles.end() && rendererIt->second == handle)
    {
        m_rendererHandles.erase(rendererIt);
    }

    entry.renderer = nullptr;
    entry.alive = false;
    entry.dirtyQueued = false;
    entry.skinned = false;

    ++entry.generation;

    if (entry.generation == 0)
    {
        entry.generation = 1;
    }

    m_freeRegistryEntries.push_back(handle.index);

    renderer->clearShadowCasterHandle();
}

uint32_t ShadowCasterCullingPass::allocateCandidateSlot(uint32_t ownerEntryIndex, uint32_t submeshIndex)
{
    uint32_t slotIndex = UINT32_MAX;

    if (!m_freeCandidateSlots.empty())
    {
        slotIndex = m_freeCandidateSlots.back();
        m_freeCandidateSlots.pop_back();
    }
    else
    {
        slotIndex = static_cast<uint32_t>(m_candidateSlots.size());
        m_candidateSlots.emplace_back();
    }

    ShadowCandidateSlot& slot = m_candidateSlots[slotIndex];

    slot.ownerEntryIndex = ownerEntryIndex;
    slot.submeshIndex = submeshIndex;
    slot.alive = true;

    ++m_liveCandidateSlotCount;
    m_candidateSlotHighWaterMark = std::max(m_candidateSlotHighWaterMark, slotIndex + 1);

    return slotIndex;
}

bool ShadowCasterCullingPass::isRegistryHandleAlive(const ShadowCasterHandle& handle) const
{
    if (!handle.isFormed() || handle.registryId != m_registryId || handle.index >= m_registryEntries.size())
    {
        return false;
    }

    const ShadowCasterRegistryEntry& entry = m_registryEntries[handle.index];

    return entry.alive && entry.generation == handle.generation;
}

void ShadowCasterCullingPass::markRendererDirty(const ShadowCasterHandle& handle)
{
    if (!isRegistryHandleAlive(handle))
    {
        return;
    }

    ShadowCasterRegistryEntry& entry = m_registryEntries[handle.index];

    if (entry.dirtyQueued)
    {
        return;
    }

    entry.dirtyQueued = true;
    m_dirtyHandles.push_back(handle);
}

bool ShadowCasterCullingPass::buildPersistentCandidate(uint32_t slotIndex, ShadowCasterCandidateGPU& candidate) const
{
    candidate = {};

    if (slotIndex >= m_candidateSlots.size())
    {
        return false;
    }

    const ShadowCandidateSlot& slot = m_candidateSlots[slotIndex];

    if (!slot.alive || slot.ownerEntryIndex >= m_registryEntries.size())
    {
        return false;
    }

    const ShadowCasterRegistryEntry& entry = m_registryEntries[slot.ownerEntryIndex];
    MeshRenderer* renderer = entry.renderer;

    if (!entry.alive || renderer == nullptr || !renderer->isActive() || !renderer->hasMesh() || !renderer->getCastShadows())
    {
        return false;
    }

    GameObject* owner = renderer->getOwner();
    Transform* transform = renderer->getTransform();

    if (owner == nullptr || !owner->IsActiveInWindowHierarchy() || transform == nullptr)
    {
        return false;
    }

    const std::shared_ptr<BasicMesh>& mesh = renderer->getMesh();

    if (mesh == nullptr || !mesh->hasIndexBuffer())
    {
        return false;
    }

    const std::vector<Submesh>& submeshes = mesh->getSubmeshes();

    if (slot.submeshIndex >= submeshes.size())
    {
        return false;
    }

    const Skin* skin = renderer->getSkin();

    const VertexBuffer* gpuSkinnedVB = skin != nullptr ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* cpuSkinnedVB = skin != nullptr && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

    const bool useGpuSkinnedVB = gpuSkinnedVB != nullptr;
    const bool useCpuSkinnedVB = !useGpuSkinnedVB && cpuSkinnedVB != nullptr;
    const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

    const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : useCpuSkinnedVB ? cpuSkinnedVB : staticVB;

    if (activeVB == nullptr)
    {
        return false;
    }

    const Matrix model = useWorldSpaceSkinnedVB ? Matrix::Identity : transform->getGlobalMatrix();
    const Matrix modelTranspose = model.Transpose();

    const Vector3* bounds = renderer->getBoundingBox().getPoints();

    for (uint32_t i = 0; i < 8; ++i)
    {
        candidate.bounds[i][0] = bounds[i].x;
        candidate.bounds[i][1] = bounds[i].y;
        candidate.bounds[i][2] = bounds[i].z;
        candidate.bounds[i][3] = 1.0f;
    }

    storeMatrix(modelTranspose, candidate.model);

    const D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
    const D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();

    splitGpuAddress(vbv.BufferLocation, candidate.vertexBufferAddressLow, candidate.vertexBufferAddressHigh);
    candidate.vertexBufferSize = vbv.SizeInBytes;
    candidate.vertexBufferStride = vbv.StrideInBytes;

    splitGpuAddress(ibv.BufferLocation, candidate.indexBufferAddressLow, candidate.indexBufferAddressHigh);
    candidate.indexBufferSize = ibv.SizeInBytes;
    candidate.indexBufferFormat = static_cast<uint32_t>(ibv.Format);

    const Submesh& submesh = submeshes[slot.submeshIndex];

    candidate.indexCountPerInstance = static_cast<uint32_t>(submesh.indexCount);
    candidate.instanceCount = 1;
    candidate.startIndexLocation = static_cast<uint32_t>(submesh.indexStart);
    candidate.baseVertexLocation = 0;
    candidate.startInstanceLocation = 0;

    candidate.flags = useWorldSpaceSkinnedVB ? SHADOW_CASTER_FLAG_FORCE_VISIBLE : 0u;

    return true;
}

void ShadowCasterCullingPass::preparePersistentCandidateBuffer(const RenderContext& ctx)
{
    if (!m_registryInitialized || m_candidateSlotHighWaterMark == 0 || ctx.ringBuffer == nullptr)
    {
        return;
    }

    if (!m_persistentCandidateBuffer || m_candidateSlotHighWaterMark > m_persistentCandidateCapacity)
    {
        uint32_t newCapacity = std::max(1024u, m_persistentCandidateCapacity);

        while (newCapacity < m_candidateSlotHighWaterMark)
        {
            newCapacity *= 2;
        }

        if (m_persistentCandidateBuffer)
        {
            app->getModuleResources()->deferResourceRelease(std::move(m_persistentCandidateBuffer));
        }

        m_persistentCandidateBuffer = app->getModuleResources()->createDefaultBuffer(
            static_cast<size_t>(newCapacity) * sizeof(ShadowCasterCandidateGPU),
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_COPY_DEST,
            "ShadowPersistentCandidates");

        m_persistentCandidateCapacity = newCapacity;
        m_persistentCandidateBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
        m_persistentCandidateNeedsFullUpload = true;
    }

    if (!m_persistentCandidateNeedsFullUpload)
    {
        return;
    }

    m_pendingPersistentCandidateCopies.clear();
    m_pendingPersistentUpdatedSlotCount = 0;
    m_pendingPersistentUpdatedBytes = 0;

    std::vector<ShadowCasterCandidateGPU> snapshot(m_candidateSlotHighWaterMark);

    uint32_t populatedCount = 0;

    for (uint32_t slotIndex = 0; slotIndex < m_candidateSlotHighWaterMark; ++slotIndex)
    {
        if (buildPersistentCandidate(slotIndex, snapshot[slotIndex]))
        {
            ++populatedCount;
        }
    }

    const size_t uploadSize = snapshot.size() * sizeof(ShadowCasterCandidateGPU);
    const RingBufferAllocation allocation = ctx.ringBuffer->allocateWithInfo(snapshot.data(), uploadSize);

    if (!allocation.isValid())
    {
        DEBUG_WARN("[Shadow Persistent] Failed to allocate full candidate upload.");
        return;
    }

    m_pendingPersistentUploadResource = allocation.resource;
    m_pendingPersistentUploadOffset = static_cast<uint64_t>(allocation.offset);
    m_pendingPersistentUploadSize = static_cast<uint64_t>(allocation.size);
    m_pendingPersistentCandidateCount = m_candidateSlotHighWaterMark;
    m_pendingPersistentPopulatedCount = populatedCount;
}

void ShadowCasterCullingPass::transitionPersistentCandidateBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState)
{
    if (commandList == nullptr || !m_persistentCandidateBuffer || m_persistentCandidateBufferState == newState)
    {
        return;
    }

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_persistentCandidateBuffer.Get(), m_persistentCandidateBufferState, newState);

    commandList->ResourceBarrier(1, &barrier);
    m_persistentCandidateBufferState = newState;
}

void ShadowCasterCullingPass::uploadPendingPersistentCandidates(ID3D12GraphicsCommandList4* commandList)
{
    if (commandList == nullptr || !m_persistentCandidateBuffer)
    {
        return;
    }

    const bool hasFullUpload = m_pendingPersistentUploadResource != nullptr && m_pendingPersistentUploadSize > 0;
    const bool hasPartialUploads = !m_pendingPersistentCandidateCopies.empty();

    if (!hasFullUpload && !hasPartialUploads)
    {
        return;
    }

    transitionPersistentCandidateBuffer(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

    if (hasFullUpload)
    {
        commandList->CopyBufferRegion(
            m_persistentCandidateBuffer.Get(),
            0,
            m_pendingPersistentUploadResource,
            m_pendingPersistentUploadOffset,
            m_pendingPersistentUploadSize);

        transitionPersistentCandidateBuffer(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        DEBUG_LOG(
            "[Shadow Persistent] Full upload | Slots: %u | Populated: %u | Capacity: %u | Upload: %.2f MiB",
            m_pendingPersistentCandidateCount,
            m_pendingPersistentPopulatedCount,
            m_persistentCandidateCapacity,
            static_cast<double>(m_pendingPersistentUploadSize) / (1024.0 * 1024.0));

        m_pendingPersistentUploadResource = nullptr;
        m_pendingPersistentUploadOffset = 0;
        m_pendingPersistentUploadSize = 0;
        m_pendingPersistentCandidateCount = 0;
        m_pendingPersistentPopulatedCount = 0;

        m_pendingPersistentCandidateCopies.clear();
        m_pendingPersistentUpdatedSlotCount = 0;
        m_pendingPersistentUpdatedBytes = 0;

        m_persistentCandidateNeedsFullUpload = false;
        return;
    }

    for (const PendingPersistentCandidateCopy& copy : m_pendingPersistentCandidateCopies)
    {
        if (copy.sourceResource == nullptr || copy.size == 0)
        {
            continue;
        }

        commandList->CopyBufferRegion(
            m_persistentCandidateBuffer.Get(),
            copy.destinationOffset,
            copy.sourceResource,
            copy.sourceOffset,
            copy.size);
    }

    transitionPersistentCandidateBuffer(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    ++m_persistentPartialUploadLogCounter;

    if (m_persistentPartialUploadLogCounter >= 60)
    {
        DEBUG_LOG(
            "[Shadow Persistent] Partial updates | Slots: %u | Copies: %zu | Upload: %.2f KiB",
            m_pendingPersistentUpdatedSlotCount,
            m_pendingPersistentCandidateCopies.size(),
            static_cast<double>(m_pendingPersistentUpdatedBytes) / 1024.0);

        m_persistentPartialUploadLogCounter = 0;
    }

    m_pendingPersistentCandidateCopies.clear();
    m_pendingPersistentUpdatedSlotCount = 0;
    m_pendingPersistentUpdatedBytes = 0;
}

void ShadowCasterCullingPass::buildCandidates()
{
    m_candidates.clear();

    for (MeshRenderer* renderer : app->getModuleScene()->getMeshRenderers())
    {
        ++m_preparationStats.visitedRenderers;

        if (renderer == nullptr || !renderer->isActive() || !renderer->hasMesh() || !renderer->getCastShadows()) continue;

        GameObject* owner = renderer->getOwner();
        Transform* transform = renderer->getTransform();

        if (owner == nullptr || !owner->IsActiveInWindowHierarchy() || transform == nullptr) continue;

        const std::shared_ptr<BasicMesh>& mesh = renderer->getMesh();

        if (mesh == nullptr || !mesh->hasIndexBuffer()) continue;

        const Skin* skin = renderer->getSkin();

        const VertexBuffer* gpuSkinnedVB = skin != nullptr ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
        const VertexBuffer* cpuSkinnedVB = skin != nullptr && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;
        const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

        const bool useGpuSkinnedVB = gpuSkinnedVB != nullptr;
        const bool useCpuSkinnedVB = !useGpuSkinnedVB && cpuSkinnedVB != nullptr;
        const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

        const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : useCpuSkinnedVB ? cpuSkinnedVB : staticVB;

        if (activeVB == nullptr) continue;

        ++m_preparationStats.eligibleRenderers;

        if (useWorldSpaceSkinnedVB)
        {
            ++m_preparationStats.skinnedRenderers;
            m_preparationStats.skinnedCandidateCount += static_cast<uint32_t>(mesh->getSubmeshes().size());
        }

        const Matrix model = useWorldSpaceSkinnedVB ? Matrix::Identity : transform->getGlobalMatrix();
        const Matrix modelTranspose = model.Transpose();

        const Vector3* bounds = renderer->getBoundingBox().getPoints();

        const D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
        const D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();

        for (const Submesh& submesh : mesh->getSubmeshes())
        {
            ShadowCasterCandidateGPU candidate{};

            for (uint32_t i = 0; i < 8; ++i)
            {
                candidate.bounds[i][0] = bounds[i].x;
                candidate.bounds[i][1] = bounds[i].y;
                candidate.bounds[i][2] = bounds[i].z;
                candidate.bounds[i][3] = 1.0f;
            }

            storeMatrix(modelTranspose, candidate.model);

            splitGpuAddress(vbv.BufferLocation, candidate.vertexBufferAddressLow, candidate.vertexBufferAddressHigh);
            candidate.vertexBufferSize = vbv.SizeInBytes;
            candidate.vertexBufferStride = vbv.StrideInBytes;

            splitGpuAddress(ibv.BufferLocation, candidate.indexBufferAddressLow, candidate.indexBufferAddressHigh);
            candidate.indexBufferSize = ibv.SizeInBytes;
            candidate.indexBufferFormat = static_cast<uint32_t>(ibv.Format);

            candidate.indexCountPerInstance = static_cast<uint32_t>(submesh.indexCount);
            candidate.instanceCount = 1;
            candidate.startIndexLocation = static_cast<uint32_t>(submesh.indexStart);
            candidate.baseVertexLocation = 0;
            candidate.startInstanceLocation = 0;

            candidate.flags = useWorldSpaceSkinnedVB ? SHADOW_CASTER_FLAG_FORCE_VISIBLE : 0u;

            m_candidates.push_back(candidate);
        }
    }
}

void ShadowCasterCullingPass::ensureVisibilityMaskCapacity(uint32_t requiredCount)
{
    if (requiredCount == 0 || requiredCount <= m_visibilityMaskCapacity) return;

    uint32_t newCapacity = std::max(1024u, m_visibilityMaskCapacity);

    while (newCapacity < requiredCount)
    {
        newCapacity *= 2;
    }

    if (m_visibilityMaskBuffer)
    {
        app->getModuleResources()->deferResourceRelease(std::move(m_visibilityMaskBuffer));
    }

    m_visibilityMaskBuffer = app->getModuleResources()->createDefaultBuffer(
        static_cast<size_t>(newCapacity) * sizeof(uint32_t),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        "ShadowCasterVisibilityMasks");

    m_visibilityMaskCapacity = newCapacity;
    m_visibilityMaskBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

void ShadowCasterCullingPass::ensureIndirectCommandCapacity(uint32_t requiredCount)
{
    if (requiredCount == 0 || requiredCount <= m_indirectCommandCapacity) return;

    uint32_t newCapacity = std::max(1024u, m_indirectCommandCapacity);

    while (newCapacity < requiredCount)
    {
        newCapacity *= 2;
    }

    const size_t bufferSize = static_cast<size_t>(newCapacity) * sizeof(ShadowIndirectCommandGPU);

    for (uint32_t cascadeIndex = 0; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex)
    {
        if (m_indirectCommandBuffers[cascadeIndex])
        {
            app->getModuleResources()->deferResourceRelease(std::move(m_indirectCommandBuffers[cascadeIndex]));
        }

        const std::string name = "ShadowIndirectCommands_" + std::to_string(cascadeIndex);

        m_indirectCommandBuffers[cascadeIndex] = app->getModuleResources()->createDefaultBuffer(
            bufferSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            name.c_str());

        m_indirectCommandBufferStates[cascadeIndex] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    m_indirectCommandCapacity = newCapacity;
}

void ShadowCasterCullingPass::transitionIndirectCommandBuffer(ID3D12GraphicsCommandList4* commandList, uint32_t cascadeIndex, D3D12_RESOURCE_STATES newState)
{
    if (!commandList || cascadeIndex >= MAX_SHADOW_CASCADES) return;

    ComPtr<ID3D12Resource>& buffer = m_indirectCommandBuffers[cascadeIndex];
    D3D12_RESOURCE_STATES& state = m_indirectCommandBufferStates[cascadeIndex];

    if (!buffer || state == newState) return;

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), state, newState);
    commandList->ResourceBarrier(1, &barrier);

    state = newState;
}

void ShadowCasterCullingPass::transitionVisibilityMaskBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState)
{
    if (!commandList || !m_visibilityMaskBuffer || m_visibilityMaskBufferState == newState) return;

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_visibilityMaskBuffer.Get(), m_visibilityMaskBufferState, newState);
    commandList->ResourceBarrier(1, &barrier);

    m_visibilityMaskBufferState = newState;
}

void ShadowCasterCullingPass::transitionCascadeCountBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState)
{
    if (!commandList || !m_cascadeCountBuffer || m_cascadeCountBufferState == newState) return;

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_cascadeCountBuffer.Get(), m_cascadeCountBufferState, newState);
    commandList->ResourceBarrier(1, &barrier);

    m_cascadeCountBufferState = newState;
}

void ShadowCasterCullingPass::reconcileCandidateSlots(uint32_t entryIndex, ShadowCasterRegistryEntry& entry)
{
    uint32_t desiredSlotCount = 0;

    if (entry.renderer != nullptr)
    {
        const std::shared_ptr<BasicMesh>& mesh = entry.renderer->getMesh();

        if (mesh != nullptr)
        {
            desiredSlotCount = static_cast<uint32_t>(mesh->getSubmeshes().size());
        }
    }

    while (entry.candidateSlots.size() > desiredSlotCount)
    {
        releaseCandidateSlot(entry.candidateSlots.back());
        entry.candidateSlots.pop_back();
    }

    while (entry.candidateSlots.size() < desiredSlotCount)
    {
        const uint32_t submeshIndex = static_cast<uint32_t>(entry.candidateSlots.size());
        entry.candidateSlots.push_back(allocateCandidateSlot(entryIndex, submeshIndex));
    }
}

void ShadowCasterCullingPass::reconcileSkinnedMembership(const ShadowCasterHandle& handle, ShadowCasterRegistryEntry& entry)
{
    const bool shouldBeSkinned = entry.renderer != nullptr && entry.renderer->hasSkinningConfiguration();

    if (shouldBeSkinned == entry.skinned)
    {
        return;
    }

    if (shouldBeSkinned)
    {
        entry.skinned = true;
        entry.skinnedListIndex = static_cast<uint32_t>(m_skinnedHandles.size());
        m_skinnedHandles.push_back(handle);
        return;
    }

    removeSkinnedHandle(entry);
}

void ShadowCasterCullingPass::releaseCandidateSlot(uint32_t slotIndex)
{
    if (slotIndex >= m_candidateSlots.size())
    {
        return;
    }

    ShadowCandidateSlot& slot = m_candidateSlots[slotIndex];

    if (!slot.alive)
    {
        return;
    }

    slot.ownerEntryIndex = UINT32_MAX;
    slot.submeshIndex = UINT32_MAX;
    slot.alive = false;

    m_retiredCandidateSlots.push_back(slotIndex);
    m_freeCandidateSlots.push_back(slotIndex);

    if (m_liveCandidateSlotCount > 0)
    {
        --m_liveCandidateSlotCount;
    }
}

void ShadowCasterCullingPass::releaseCandidateSlots(ShadowCasterRegistryEntry& entry)
{
    for (uint32_t slotIndex : entry.candidateSlots)
    {
        releaseCandidateSlot(slotIndex);
    }

    entry.candidateSlots.clear();
}

void ShadowCasterCullingPass::removeSkinnedHandle(ShadowCasterRegistryEntry& entry)
{
    if (!entry.skinned || entry.skinnedListIndex == UINT32_MAX || entry.skinnedListIndex >= m_skinnedHandles.size())
    {
        entry.skinned = false;
        entry.skinnedListIndex = UINT32_MAX;
        return;
    }

    const uint32_t removeIndex = entry.skinnedListIndex;
    const ShadowCasterHandle movedHandle = m_skinnedHandles.back();

    m_skinnedHandles[removeIndex] = movedHandle;
    m_skinnedHandles.pop_back();

    if (removeIndex < m_skinnedHandles.size() && isRegistryHandleAlive(movedHandle))
    {
        m_registryEntries[movedHandle.index].skinnedListIndex = removeIndex;
    }

    entry.skinned = false;
    entry.skinnedListIndex = UINT32_MAX;
}