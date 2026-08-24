#pragma once

#include "IRenderPass.h"

#include "Lights.h"
#include "SceneDataCB.h"
#include "DamageHighlightComponent.h"
#include "DissolveComponent.h"

#include <vector>
#include <memory>

class GameObject;
class MeshRenderer;
class RenderSurface;
class LightComponent;

struct OcclusionRevealVisualEffectsCB
{
    DamageHighlightDataCB damageHighlightDataCB;
    DissolveCB dissolveDataCB;
};

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

    GPULightsConstantBuffer packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const;

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

    D3D12_GPU_VIRTUAL_ADDRESS m_sceneDataCBAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS m_lightsAddress = 0;

    D3D12_GPU_VIRTUAL_ADDRESS m_shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE m_cascadeShadowMapSRV{};
    bool m_hasShadowData = false;

    float m_depthBias = 0.00001f;
    float m_revealAlpha = 0.65f;
};