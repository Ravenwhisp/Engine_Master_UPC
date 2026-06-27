#pragma once

#include "IRenderPass.h"
#include "SSAOTypes.h"
#include "SimpleMath.h"

#include <array>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Texture;

class SSAOPass : public IRenderPass
{
public:
    explicit SSAOPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void createKernel();
    void uploadConstants(const RenderContext& ctx);

private:
    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    Texture* m_outputTexture = nullptr;
    Texture* m_depthTexture = nullptr;
    Texture* m_normalTexture = nullptr;

    std::array<DirectX::SimpleMath::Vector4, SSAO_KERNEL_SIZE> m_kernel{};

    SSAODataCB m_ssaoData{};
    D3D12_GPU_VIRTUAL_ADDRESS m_ssaoCBAddress = 0;
};