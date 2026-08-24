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
class Texture;

static constexpr UINT MAX_OCCLUSION_BUBBLES = 2;

struct OcclusionRevealVisualEffectsCB
{
    DamageHighlightDataCB damageHighlightDataCB;
    DissolveCB dissolveDataCB;
};

struct OcclusionBubbleCB
{
    DirectX::SimpleMath::Vector4 centerRadius[MAX_OCCLUSION_BUBBLES]{};
    DirectX::SimpleMath::Vector4 depthParams[MAX_OCCLUSION_BUBBLES]{};
    DirectX::SimpleMath::Vector4 settings = DirectX::SimpleMath::Vector4::Zero;
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

    void createBubbleRootSignature();
    void createBubblePipelineState();

    void collectMeshRenderers(GameObject* gameObject);
    void renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer);

    bool buildBubbleForTarget(GameObject* targetRoot, UINT bubbleIndex);
    void accumulateProjectedBounds(GameObject* gameObject, float& minX, float& minY, float& maxX, float& maxY, float& minDepth, float& maxDepth, bool& hasProjectedPoint) const;
    void renderBubble(ID3D12GraphicsCommandList4* commandList, Texture* mainDepth, Texture* eligibility, D3D12_CPU_DESCRIPTOR_HANDLE rtv);

    GPULightsConstantBuffer packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const;

private:
    ComPtr<ID3D12Device4> m_device;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    ComPtr<ID3D12RootSignature> m_bubbleRootSignature;
    ComPtr<ID3D12PipelineState> m_bubblePipelineState;

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

    OcclusionBubbleCB m_bubbleCB{};
    D3D12_GPU_VIRTUAL_ADDRESS m_bubbleCBAddress = 0;

    float m_depthBias = 0.00001f;
    float m_revealAlpha = 0.65f;

    // Temporary artistic values.
    // These move to OcclusionTargetComponent/settings in the final polish commit.
    float m_bubbleScale = 1.35f;
    float m_bubbleSoftness = 0.35f;
    float m_occluderOpacity = 0.65f;
};