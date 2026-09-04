#pragma once

#include "IRenderPass.h"
#include "FlowMapGPUData.h"

#include <vector>

class MeshRenderer;
class RenderSurface;

class FlowMapPass : public IRenderPass
{
public:
    explicit FlowMapPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void createRootSignature();
    void createPipelineState();
    void transitionGBuffer(ID3D12GraphicsCommandList4* commandList,
        D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) const;
    void renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer);

    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    std::vector<MeshRenderer*> m_meshRenderers;
    RenderSurface* m_renderSurface = nullptr;
    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;
};
