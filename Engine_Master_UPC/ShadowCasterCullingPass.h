#pragma once

#include "IRenderPass.h"
#include "ShadowCasterTypes.h"

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class ShadowFrustumComputePass;

class ShadowCasterCullingPass : public IRenderPass
{
public:
    ShadowCasterCullingPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass);
    ~ShadowCasterCullingPass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    D3D12_GPU_VIRTUAL_ADDRESS getCandidateBufferAddress() const { return m_candidateBufferAddress; }
    uint32_t getCandidateCount() const { return m_candidateCount; }

private:
    void createRootSignature();
    void createPipelineState();
    void createCounterResources();
    void createDebugReadbackBuffers();

    void buildCandidates();
    void ensureVisibilityMaskCapacity(uint32_t requiredCount);

    void transitionVisibilityMaskBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);
    void transitionCascadeCountBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);

    void refreshDebugReadback();
    void recordDebugReadback(ID3D12GraphicsCommandList4* commandList);

private:
    ComPtr<ID3D12Device4> m_device;
    ShadowFrustumComputePass* m_shadowFrustumComputePass = nullptr;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::vector<ShadowCasterCandidateGPU> m_candidates;

    D3D12_GPU_VIRTUAL_ADDRESS m_candidateBufferAddress = 0;
    uint32_t m_candidateCount = 0;

    ComPtr<ID3D12Resource> m_visibilityMaskBuffer;
    uint32_t m_visibilityMaskCapacity = 0;
    D3D12_RESOURCE_STATES m_visibilityMaskBufferState = D3D12_RESOURCE_STATE_COMMON;

    ComPtr<ID3D12Resource> m_cascadeCountBuffer;
    ComPtr<ID3D12Resource> m_zeroCountUploadBuffer;
    D3D12_RESOURCE_STATES m_cascadeCountBufferState = D3D12_RESOURCE_STATE_COPY_DEST;

    std::vector<ComPtr<ID3D12Resource>> m_debugReadbackBuffers;
    std::vector<bool> m_debugReadbackPending;

    bool m_captureDebugReadback = false;
};