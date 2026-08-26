#pragma once

#include "IRenderPass.h"

class RenderSurface;

class DynamicTransparencyMaskPass : public IRenderPass
{
public:
    explicit DynamicTransparencyMaskPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void createRootSignature();
    void createPipelineState();

private:
    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    RenderSurface* m_renderSurface = nullptr;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
};