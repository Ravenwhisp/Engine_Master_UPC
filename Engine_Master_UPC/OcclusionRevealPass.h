#pragma once

#include "IRenderPass.h"

#include <vector>

class GameObject;
class MeshRenderer;
class RenderSurface;

class OcclusionRevealPass : public IRenderPass
{
public:
    OcclusionRevealPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void createRootSignature();
    void createPipelineState();

    void collectMeshRenderers(GameObject* gameObject);
    void renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer);

private:
    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::vector<MeshRenderer*> m_meshRenderers;

    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};

    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;

    RenderSurface* m_renderSurface = nullptr;

    float m_depthBias = 0.00001f;
};
