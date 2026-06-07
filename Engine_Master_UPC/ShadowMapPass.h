#pragma once

#include "IRenderPass.h"
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

class ShadowMapPass : public IRenderPass
{
public:
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;

    struct ShadowDrawConstants
    {
        Matrix mvp = Matrix::Identity;
    };

public:
    explicit ShadowMapPass(ComPtr<ID3D12Device4> device);
    ~ShadowMapPass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    const Texture* getShadowMap() const { return m_shadowMap.get(); }
    Texture* getShadowMap() { return m_shadowMap.get(); }

    const ShadowFrameData& getFrameData() const { return m_frameData; }

private:
    void createRootSignature();
    void createPipelineState();

    const LightComponent* findMainDirectionalLight() const;
    void prepareDisabledShadowData(const RenderContext& ctx);
    void prepareDirectionalShadowData(const RenderContext& ctx, const LightComponent& light);

    void renderCasters(ID3D12GraphicsCommandList4* commandList);
    void renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer& renderer);

private:
    static constexpr float SHADOW_ORTHO_SIZE = 80.0f;
    static constexpr float SHADOW_LIGHT_DISTANCE = 100.0f;
    static constexpr float SHADOW_NEAR_PLANE = 0.1f;
    static constexpr float SHADOW_FAR_PLANE = 200.0f;
    static constexpr float SHADOW_BIAS = 0.0005f;
    static constexpr float SHADOW_STRENGTH = 1.0f;

private:
    ComPtr<ID3D12Device4> m_device;

    std::unique_ptr<Texture> m_shadowMap;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    ShadowFrameData m_frameData{};
    std::vector<MeshRenderer*> m_meshRenderers;
};