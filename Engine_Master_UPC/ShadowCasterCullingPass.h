#pragma once

#include "IRenderPass.h"
#include "ShadowCasterTypes.h"
#include "ShadowTypes.h"
#include "ShadowCasterHandle.h"

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <array>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

class ShadowFrustumComputePass;
class MeshRenderer;
class Scene;

class ShadowCasterCullingPass : public IRenderPass
{
public:
    ShadowCasterCullingPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass);
    ~ShadowCasterCullingPass() override = default;

    struct PreparationStats
    {
        float buildMs = 0.0f;
        float capacityMs = 0.0f;
        float uploadMs = 0.0f;
        float totalMs = 0.0f;

        uint32_t visitedRenderers = 0;
        uint32_t eligibleRenderers = 0;
        uint32_t skinnedRenderers = 0;
        uint32_t candidateCount = 0;
        uint32_t skinnedCandidateCount = 0;

        uint64_t uploadBytes = 0;
    };

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    D3D12_GPU_VIRTUAL_ADDRESS getCandidateBufferAddress() const { return m_candidateBufferAddress; }
    uint32_t getCandidateCount() const { return m_candidateCount; }

    ID3D12Resource* getIndirectCommandBuffer(uint32_t cascadeIndex) const { return cascadeIndex < MAX_SHADOW_CASCADES ? m_indirectCommandBuffers[cascadeIndex].Get() : nullptr; }
    ID3D12Resource* getCascadeCountBuffer() const { return m_cascadeCountBuffer.Get(); }

    const PreparationStats& getPreparationStats() const { return m_preparationStats; }

    ShadowCasterHandle registerRenderer(MeshRenderer* renderer);
    void unregisterRenderer(MeshRenderer* renderer, const ShadowCasterHandle& handle);
    bool isRegistryHandleAlive(const ShadowCasterHandle& handle) const;
    void markRendererDirty(const ShadowCasterHandle& handle);

private:

    struct ShadowCasterRegistryEntry
    {
        MeshRenderer* renderer = nullptr;
        uint32_t generation = 1;
        std::vector<uint32_t> candidateSlots;

        bool alive = false;
        bool dirtyQueued = false;
        bool skinned = false;
        uint64_t lastProcessedRevision = 0;
        uint32_t skinnedListIndex = UINT32_MAX;
    };

    struct ShadowCandidateSlot
    {
        uint32_t ownerEntryIndex = UINT32_MAX;
        uint32_t submeshIndex = UINT32_MAX;
        bool alive = false;
    };

    struct PendingPersistentCandidateCopy
    {
        ID3D12Resource* sourceResource = nullptr;
        uint64_t sourceOffset = 0;
        uint64_t destinationOffset = 0;
        uint64_t size = 0;
    };

    void createRootSignature();
    void createPipelineState();
    void createCounterResources();

    void buildCandidates();
    void ensureRegistryBootstrap();
    void resetRegistry();
    void initializeRegistry();
    void processDirtyQueue(const RenderContext& ctx);

    bool buildPersistentCandidate(uint32_t slotIndex, ShadowCasterCandidateGPU& candidate) const;
    void preparePersistentCandidateBuffer(const RenderContext& ctx);
    void uploadPendingPersistentCandidates(ID3D12GraphicsCommandList4* commandList);
    void transitionPersistentCandidateBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);
    void queuePersistentCandidateUpdates(const std::vector<uint32_t>& slotIndices, const RenderContext& ctx);

    void ensureVisibilityMaskCapacity(uint32_t requiredCount);
    void ensureIndirectCommandCapacity(uint32_t requiredCount);
    void transitionIndirectCommandBuffer(ID3D12GraphicsCommandList4* commandList, uint32_t cascadeIndex, D3D12_RESOURCE_STATES newState);

    void transitionVisibilityMaskBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);
    void transitionCascadeCountBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);

    void reconcileCandidateSlots(uint32_t entryIndex, ShadowCasterRegistryEntry& entry);
    void reconcileSkinnedMembership(const ShadowCasterHandle& handle, ShadowCasterRegistryEntry& entry);

    void releaseCandidateSlot(uint32_t slotIndex);
    void releaseCandidateSlots(ShadowCasterRegistryEntry& entry);
    void removeSkinnedHandle(ShadowCasterRegistryEntry& entry);

    uint32_t allocateCandidateSlot(uint32_t ownerEntryIndex, uint32_t submeshIndex);

private:
    ComPtr<ID3D12Device4> m_device;
    ShadowFrustumComputePass* m_shadowFrustumComputePass = nullptr;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::vector<ShadowCasterCandidateGPU> m_candidates;

    D3D12_GPU_VIRTUAL_ADDRESS m_candidateBufferAddress = 0;
    uint32_t m_candidateCount = 0;
    uint32_t m_candidateFrameIndex = UINT32_MAX;
    uint64_t m_candidateFenceValue = UINT64_MAX;

    ComPtr<ID3D12Resource> m_visibilityMaskBuffer;
    uint32_t m_visibilityMaskCapacity = 0;
    D3D12_RESOURCE_STATES m_visibilityMaskBufferState = D3D12_RESOURCE_STATE_COMMON;

    ComPtr<ID3D12Resource> m_cascadeCountBuffer;
    ComPtr<ID3D12Resource> m_zeroCountUploadBuffer;
    D3D12_RESOURCE_STATES m_cascadeCountBufferState = D3D12_RESOURCE_STATE_COPY_DEST;

    std::array<ComPtr<ID3D12Resource>, MAX_SHADOW_CASCADES> m_indirectCommandBuffers;
    std::array<D3D12_RESOURCE_STATES, MAX_SHADOW_CASCADES> m_indirectCommandBufferStates{};
    uint32_t m_indirectCommandCapacity = 0;

    PreparationStats m_preparationStats{};
    uint32_t m_preparationProfileLogCounter = 0;

    Scene* m_registryScene = nullptr;
    bool m_registryInitialized = false;

    std::vector<ShadowCasterRegistryEntry> m_registryEntries;
    std::vector<ShadowCandidateSlot> m_candidateSlots;

    std::unordered_map<MeshRenderer*, ShadowCasterHandle> m_rendererHandles;

    std::vector<uint32_t> m_freeRegistryEntries;
    std::vector<uint32_t> m_freeCandidateSlots;

    std::vector<ShadowCasterHandle> m_dirtyHandles;
    std::vector<ShadowCasterHandle> m_skinnedHandles;

    uint32_t m_candidateSlotHighWaterMark = 0;
    uint32_t m_liveCandidateSlotCount = 0;
    uint64_t m_registryId = 0;

    ComPtr<ID3D12Resource> m_persistentCandidateBuffer;
    uint32_t m_persistentCandidateCapacity = 0;
    D3D12_RESOURCE_STATES m_persistentCandidateBufferState = D3D12_RESOURCE_STATE_COPY_DEST;

    ID3D12Resource* m_pendingPersistentUploadResource = nullptr;
    uint64_t m_pendingPersistentUploadOffset = 0;
    uint64_t m_pendingPersistentUploadSize = 0;

    bool m_persistentCandidateNeedsFullUpload = true;
    uint32_t m_pendingPersistentCandidateCount = 0;
    uint32_t m_pendingPersistentPopulatedCount = 0;

    std::vector<PendingPersistentCandidateCopy> m_pendingPersistentCandidateCopies;

    uint32_t m_pendingPersistentUpdatedSlotCount = 0;
    uint64_t m_pendingPersistentUpdatedBytes = 0;
    uint32_t m_persistentPartialUploadLogCounter = 0;

};