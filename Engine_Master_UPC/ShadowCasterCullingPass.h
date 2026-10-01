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
    void buildCandidates();

private:
    ComPtr<ID3D12Device4> m_device;
    ShadowFrustumComputePass* m_shadowFrustumComputePass = nullptr;

    std::vector<ShadowCasterCandidateGPU> m_candidates;

    D3D12_GPU_VIRTUAL_ADDRESS m_candidateBufferAddress = 0;
    uint32_t m_candidateCount = 0;
};