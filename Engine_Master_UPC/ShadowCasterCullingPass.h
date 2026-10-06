#pragma once

#include "IRenderPass.h"
#include "ShadowCasterTypes.h"
#include "ShadowTypes.h"
#include "ShadowCasterHandle.h"

#include <array>
#include <cstdint>
#include <d3d12.h>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class MeshRenderer;
class Scene;
class ShadowFrustumComputePass;

/*
 * GPU shadow-caster culling pass.
 *
 * Maintains a persistent shadow candidate registry with stable slots.
 * Static candidates are updated only when dirty, while skinned candidates
 * are refreshed each frame. The GPU culls the persistent candidate set
 * against each shadow cascade and generates compacted indirect commands.
 */
class ShadowCasterCullingPass : public IRenderPass
{
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

private:
    // Core pipeline
    ComPtr<ID3D12Device4> m_device;
    ShadowFrustumComputePass* m_shadowFrustumComputePass = nullptr;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    // Current candidate set
    D3D12_GPU_VIRTUAL_ADDRESS m_candidateBufferAddress = 0;
    uint32_t m_candidateCount = 0;
    uint32_t m_candidateFrameIndex = UINT32_MAX;
    uint64_t m_candidateFenceValue = UINT64_MAX;

    // GPU culling output
    ComPtr<ID3D12Resource> m_visibilityMaskBuffer;
    uint32_t m_visibilityMaskCapacity = 0;
    D3D12_RESOURCE_STATES m_visibilityMaskBufferState = D3D12_RESOURCE_STATE_COMMON;

    ComPtr<ID3D12Resource> m_cascadeCountBuffer;
    ComPtr<ID3D12Resource> m_zeroCountUploadBuffer;
    D3D12_RESOURCE_STATES m_cascadeCountBufferState = D3D12_RESOURCE_STATE_COPY_DEST;

    std::array<ComPtr<ID3D12Resource>, MAX_SHADOW_CASCADES> m_indirectCommandBuffers;
    std::array<D3D12_RESOURCE_STATES, MAX_SHADOW_CASCADES> m_indirectCommandBufferStates{};
    uint32_t m_indirectCommandCapacity = 0;

    // Persistent registry
    Scene* m_registryScene = nullptr;
    bool m_registryInitialized = false;
    uint64_t m_registryId = 0;

    std::vector<ShadowCasterRegistryEntry> m_registryEntries;
    std::vector<ShadowCandidateSlot> m_candidateSlots;
    std::unordered_map<MeshRenderer*, ShadowCasterHandle> m_rendererHandles;

    std::vector<uint32_t> m_freeRegistryEntries;
    std::vector<uint32_t> m_freeCandidateSlots;
    std::vector<uint32_t> m_retiredCandidateSlots;

    std::vector<ShadowCasterHandle> m_dirtyHandles;
    std::vector<ShadowCasterHandle> m_skinnedHandles;

    uint32_t m_candidateSlotHighWaterMark = 0;
    uint32_t m_liveCandidateSlotCount = 0;

    // Persistent GPU candidate buffer
    ComPtr<ID3D12Resource> m_persistentCandidateBuffer;
    uint32_t m_persistentCandidateCapacity = 0;
    D3D12_RESOURCE_STATES m_persistentCandidateBufferState = D3D12_RESOURCE_STATE_COPY_DEST;

    bool m_persistentCandidateNeedsFullUpload = true;

    ID3D12Resource* m_pendingPersistentUploadResource = nullptr;
    uint64_t m_pendingPersistentUploadOffset = 0;
    uint64_t m_pendingPersistentUploadSize = 0;

    uint32_t m_pendingPersistentCandidateCount = 0;
    uint32_t m_pendingPersistentPopulatedCount = 0;

    std::vector<PendingPersistentCandidateCopy> m_pendingPersistentCandidateCopies;

private:
    // Pipeline creation
    void createRootSignature();
    void createPipelineState();
    void createCounterResources();

    // Registry lifecycle
    void ensureRegistryBootstrap();
    void resetRegistry();
    void initializeRegistry();
    void processDirtyQueue(const RenderContext& ctx);

    // Persistent candidates
    bool buildPersistentCandidate(uint32_t slotIndex, ShadowCasterCandidateGPU& candidate) const;
    void preparePersistentCandidateBuffer(const RenderContext& ctx);
    void queuePersistentCandidateUpdates(const std::vector<uint32_t>& slotIndices, const RenderContext& ctx);
    void refreshSkinnedPersistentCandidates(const RenderContext& ctx);
    void flushRetiredPersistentCandidateSlots(const RenderContext& ctx);
    void uploadPendingPersistentCandidates(ID3D12GraphicsCommandList4* commandList);
    void transitionPersistentCandidateBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);

    // Candidate slot management
    void reconcileCandidateSlots(uint32_t entryIndex, ShadowCasterRegistryEntry& entry);
    void reconcileSkinnedMembership(const ShadowCasterHandle& handle, ShadowCasterRegistryEntry& entry);
    uint32_t allocateCandidateSlot(uint32_t ownerEntryIndex, uint32_t submeshIndex);
    void releaseCandidateSlot(uint32_t slotIndex);
    void releaseCandidateSlots(ShadowCasterRegistryEntry& entry);
    void removeSkinnedHandle(ShadowCasterRegistryEntry& entry);

    // GPU output resources
    void ensureVisibilityMaskCapacity(uint32_t requiredCount);
    void ensureIndirectCommandCapacity(uint32_t requiredCount);
    void transitionVisibilityMaskBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);
    void transitionCascadeCountBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);
    void transitionIndirectCommandBuffer(ID3D12GraphicsCommandList4* commandList, uint32_t cascadeIndex, D3D12_RESOURCE_STATES newState);

public:
    // Getters
    D3D12_GPU_VIRTUAL_ADDRESS getCandidateBufferAddress() const { return m_candidateBufferAddress; }
    uint32_t getCandidateCount() const { return m_candidateCount; }
    ID3D12Resource* getCascadeCountBuffer() const { return m_cascadeCountBuffer.Get(); }
    ID3D12Resource* getIndirectCommandBuffer(uint32_t cascadeIndex) const { return cascadeIndex < MAX_SHADOW_CASCADES ? m_indirectCommandBuffers[cascadeIndex].Get() : nullptr; }

    // Lifecycle
    ShadowCasterCullingPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass);
    ~ShadowCasterCullingPass() override;

    // Render pass
    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    // Registry API
    ShadowCasterHandle registerRenderer(MeshRenderer* renderer);
    void unregisterRenderer(MeshRenderer* renderer, const ShadowCasterHandle& handle);
    void markRendererDirty(const ShadowCasterHandle& handle);
    bool isRegistryHandleAlive(const ShadowCasterHandle& handle) const;
};
