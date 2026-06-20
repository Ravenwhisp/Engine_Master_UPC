#pragma once

#include "IRenderPass.h"

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
    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    Texture* m_outputTexture = nullptr;
};