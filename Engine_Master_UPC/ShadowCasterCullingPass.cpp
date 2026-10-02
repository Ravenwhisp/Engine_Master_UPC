#include "Globals.h"
#include "ShadowCasterCullingPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleScene.h"
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
}

ShadowCasterCullingPass::ShadowCasterCullingPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass)
    : m_device(device)
    , m_shadowFrustumComputePass(shadowFrustumComputePass)
{
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

    const uint32_t frameIndex = d3d12->getCurrentFrameIndex();
    const uint64_t fenceValue = d3d12->getCurrentFrame();

    if (m_candidateFrameIndex == frameIndex && m_candidateFenceValue == fenceValue)
    {
        return;
    }

    using Clock = std::chrono::steady_clock;

    m_preparationStats = {};

    const auto totalStart = Clock::now();

    m_candidateBufferAddress = 0;
    m_candidateCount = 0;

    const auto buildStart = Clock::now();

    buildCandidates();

    const auto buildEnd = Clock::now();

    m_candidateCount = static_cast<uint32_t>(m_candidates.size());

    m_preparationStats.buildMs = std::chrono::duration<float, std::milli>(buildEnd - buildStart).count();
    m_preparationStats.candidateCount = m_candidateCount;
    m_preparationStats.uploadBytes = static_cast<uint64_t>(m_candidates.size()) * sizeof(ShadowCasterCandidateGPU);

    if (m_candidateCount > 0 && ctx.ringBuffer != nullptr)
    {
        const auto capacityStart = Clock::now();

        ensureVisibilityMaskCapacity(m_candidateCount);
        ensureIndirectCommandCapacity(m_candidateCount);

        const auto capacityEnd = Clock::now();

        m_preparationStats.capacityMs = std::chrono::duration<float, std::milli>(capacityEnd - capacityStart).count();

        const auto uploadStart = Clock::now();

        m_candidateBufferAddress = ctx.ringBuffer->allocate(m_candidates.data(), m_candidates.size() * sizeof(ShadowCasterCandidateGPU));

        const auto uploadEnd = Clock::now();

        m_preparationStats.uploadMs = std::chrono::duration<float, std::milli>(uploadEnd - uploadStart).count();
    }

    const auto totalEnd = Clock::now();

    m_preparationStats.totalMs = std::chrono::duration<float, std::milli>(totalEnd - totalStart).count();

    m_candidateFrameIndex = frameIndex;
    m_candidateFenceValue = fenceValue;

    ++m_preparationProfileLogCounter;

    if (m_preparationProfileLogCounter >= 120)
    {
        DEBUG_LOG(
            "[Shadow Candidates] Total %.3f ms | Build %.3f ms | Capacity %.3f ms | Upload %.3f ms | Renderers %u/%u | Skinned %u | Candidates %u | Skinned Candidates %u | Upload %.2f MiB",
            m_preparationStats.totalMs,
            m_preparationStats.buildMs,
            m_preparationStats.capacityMs,
            m_preparationStats.uploadMs,
            m_preparationStats.eligibleRenderers,
            m_preparationStats.visitedRenderers,
            m_preparationStats.skinnedRenderers,
            m_preparationStats.candidateCount,
            m_preparationStats.skinnedCandidateCount,
            static_cast<double>(m_preparationStats.uploadBytes) / (1024.0 * 1024.0));

        m_preparationProfileLogCounter = 0;
    }
}

void ShadowCasterCullingPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "ShadowCasterCullingPass");

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
