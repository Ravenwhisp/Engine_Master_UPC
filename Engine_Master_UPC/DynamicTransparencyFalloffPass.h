#pragma once

#include "IRenderPass.h"
#include "SimpleMath.h"

class GameObject;
class MeshRenderer;
class RenderSurface;
class DeferredShadingPass;

struct DynamicTransparencyFalloffSettingsCB
{
    DirectX::SimpleMath::Vector4 settings = DirectX::SimpleMath::Vector4::Zero;
};

static_assert(sizeof(DynamicTransparencyFalloffSettingsCB) == 16);

class DynamicTransparencyFalloffPass : public IRenderPass
{
public:
    DynamicTransparencyFalloffPass(ComPtr<ID3D12Device4> device, DeferredShadingPass* deferredShadingPass);

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

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;
    RenderSurface* m_renderSurface = nullptr;
    DeferredShadingPass* m_deferredShadingPass = nullptr;

    D3D12_GPU_VIRTUAL_ADDRESS m_sceneDataCBAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS m_lightsCBAddress = 0;

    D3D12_GPU_VIRTUAL_ADDRESS m_shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE m_cascadeShadowMapSRV{};
    bool m_hasShadowData = false;
};