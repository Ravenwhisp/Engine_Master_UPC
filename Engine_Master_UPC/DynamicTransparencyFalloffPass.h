#pragma once

#include "IRenderPass.h"
#include "SimpleMath.h"

class GameObject;
class MeshRenderer;
class RenderSurface;
class DeferredShadingPass;
class Texture;
class VolumetricFogComputePass;

struct DynamicTransparencyFalloffSettingsCB
{
    // x = depthBias
    // y = hasDissolve
    // z = dissolveAmount
    // w = fogEnabled
    DirectX::SimpleMath::Vector4 settings = DirectX::SimpleMath::Vector4::Zero;

    // x = nearDistance
    // y = maxDistance
    // z = projectionA
    // w = projectionB
    DirectX::SimpleMath::Vector4 fogDepthParams = DirectX::SimpleMath::Vector4::Zero;

    // x = gridDepth
    DirectX::SimpleMath::Vector4 fogGridParams = DirectX::SimpleMath::Vector4::Zero;
};

static_assert(sizeof(DynamicTransparencyFalloffSettingsCB) == 48);

class DynamicTransparencyFalloffPass : public IRenderPass
{
public:
    DynamicTransparencyFalloffPass(ComPtr<ID3D12Device4> device, DeferredShadingPass* deferredShadingPass, VolumetricFogComputePass* fogComputePass);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void createRootSignature();
    void createPipelineState();
    void collectMeshRenderers(GameObject* gameObject);
    void renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer);
    float getRendererSortDepth(MeshRenderer* renderer) const;

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
    VolumetricFogComputePass* m_fogComputePass = nullptr;
    Texture* m_integratedFogVolume = nullptr;

    float m_fogNearDistance = 0.0f;
    float m_fogMaxDistance = 0.0f;
    float m_fogProjectionA = 0.0f;
    float m_fogProjectionB = 0.0f;
    uint32_t m_fogGridDepth = 0;

    bool m_fogEnabled = false;

    D3D12_GPU_VIRTUAL_ADDRESS m_sceneDataCBAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS m_lightsCBAddress = 0;

    D3D12_GPU_VIRTUAL_ADDRESS m_pointLightIndexBufferAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS m_spotLightIndexBufferAddress = 0;

    D3D12_GPU_VIRTUAL_ADDRESS m_shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE m_cascadeShadowMapSRV{};
    bool m_hasShadowData = false;
};