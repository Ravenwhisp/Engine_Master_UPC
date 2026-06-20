#pragma once

#include "IRenderPass.h"

#include "SimpleMath.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class Texture;
class MeshRenderer;

class SSAOGeometryPass : public IRenderPass
{
public:
    struct Transforms
    {
        DirectX::SimpleMath::Matrix mvp;
        DirectX::SimpleMath::Matrix normalToView;
    };

    explicit SSAOGeometryPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void renderMeshes(ID3D12GraphicsCommandList* commandList);

private:
    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::vector<MeshRenderer*> m_meshRenderers;

    const DirectX::SimpleMath::Matrix* m_view = nullptr;
    const DirectX::SimpleMath::Matrix* m_projection = nullptr;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    Texture* m_depthTexture = nullptr;
    Texture* m_normalTexture = nullptr;
};