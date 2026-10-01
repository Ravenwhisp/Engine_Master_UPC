#pragma once

#include "IRenderPass.h"
#include "IDebugDrawable.h"
#include "ShadowTypes.h"
#include "Texture.h"

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

using Microsoft::WRL::ComPtr;

class LightComponent;
class MeshRenderer;

class ShadowFrustumComputePass;


class ShadowMapPass : public IRenderPass, public IDebugDrawable
{
public:

    struct ShadowDrawConstants
    {
        Matrix model = Matrix::Identity;
    };

public:
    ShadowMapPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass);
    ~ShadowMapPass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;
    void debugDraw() override;

    const Texture* getCascadeShadowMap(uint32_t index = 0) const { return m_cascadeShadowMaps.at(m_currentShadowView).at(index).get(); }
    Texture* getCascadeShadowMap(uint32_t index = 0) { return m_cascadeShadowMaps.at(m_currentShadowView).at(index).get(); }

    const ShadowFrameData& getFrameData() const { return m_frameData; }

private:
    void createRootSignature();
    void createPipelineState();

    const LightComponent* findMainShadowCastingDirectionalLight() const;
    void prepareDisabledShadowData(const RenderContext& ctx);
    void prepareDirectionalShadowData(const RenderContext& ctx, const LightComponent& light);

    void renderCasters(ID3D12GraphicsCommandList4* commandList, uint32_t cascadeIndex);
    void renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer& renderer);
    void buildShadowCasterList(const RenderContext& ctx, const LightComponent& light);

    void updateShadowViewportAndScissor(uint32_t size);

    void createCascadeShadowMap( uint32_t size, uint32_t cascadeCount);
    void resizeCascadeShadowMapIfNeeded( uint32_t size, uint32_t cascadeCount);
    void transitionCascadeShadowMap( ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);
    void transitionCascadeShadowMap(ID3D12GraphicsCommandList4* commandList, uint32_t cascadeIndex, D3D12_RESOURCE_STATES newState);

private:
    static constexpr uint32_t DEFAULT_SHADOW_MAP_SIZE = 2048;

    static constexpr float SHADOW_BIAS = 0.0005f;
    static constexpr float SHADOW_STRENGTH = 1.0f;

private:
    ComPtr<ID3D12Device4> m_device;
    ShadowFrustumComputePass* m_shadowFrustumComputePass = nullptr;

    static constexpr uint32_t SHADOW_VIEW_COUNT = 2;

    std::array<std::array<std::unique_ptr<Texture>, MAX_SHADOW_CASCADES>, SHADOW_VIEW_COUNT> m_cascadeShadowMaps;
    std::array<std::array<D3D12_RESOURCE_STATES, MAX_SHADOW_CASCADES>, SHADOW_VIEW_COUNT> m_cascadeShadowMapStates{};

    uint32_t m_currentShadowView = 0;

    uint32_t m_currentCascadeShadowMapSize = 0;
    uint32_t m_currentCascadeCount = 0;
    uint32_t m_activeCascadeCount = 0;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    ShadowFrameData m_frameData{};
    std::vector<MeshRenderer*> m_shadowCasters;
};
