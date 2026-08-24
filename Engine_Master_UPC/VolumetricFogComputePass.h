#pragma once

#include "IRenderPass.h"
#include "VolumetricFogTypes.h"

#include <d3d12.h>
#include <memory>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Texture;
class LightComponent;

class VolumetricFogComputePass final : public IRenderPass
{
public:
    explicit VolumetricFogComputePass(ComPtr<ID3D12Device4> device);
    ~VolumetricFogComputePass() override;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    const Texture* getMediumVolume() const { return m_mediumVolume.get(); }
    const Texture* getLightingVolume() const { return m_lightingVolume.get(); }
    const Texture* getIntegratedVolume() const { return m_integratedVolume.get(); }

    Texture* getMediumVolume() { return m_mediumVolume.get(); }
    Texture* getLightingVolume() { return m_lightingVolume.get(); }
    Texture* getIntegratedVolume() { return m_integratedVolume.get(); }

    bool isEnabled() const { return m_enabled; }
    const VolumetricFog::GridConstants& getGridConstants() const { return m_gridConstants; }

private:
    void createMediumRootSignature();
    void createMediumPipelineState();

    void createLightingRootSignature();
    void createLightingPipelineState();

    void createIntegrationRootSignature();
    void createIntegrationPipelineState();

    void ensureVolumes();

    const LightComponent* findVolumetricDirectionalLight() const;
    void transitionVolume(ID3D12GraphicsCommandList4* commandList, Texture* texture, D3D12_RESOURCE_STATES& currentState, D3D12_RESOURCE_STATES newState);

private:
    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_mediumRootSignature;
    ComPtr<ID3D12PipelineState> m_mediumPipelineState;
    ComPtr<ID3D12RootSignature> m_lightingRootSignature;
    ComPtr<ID3D12PipelineState> m_lightingPipelineState;
    ComPtr<ID3D12RootSignature> m_integrationRootSignature;
    ComPtr<ID3D12PipelineState> m_integrationPipelineState;

    std::unique_ptr<Texture> m_mediumVolume;
    std::unique_ptr<Texture> m_lightingVolume;
    std::unique_ptr<Texture> m_integratedVolume;

    VolumetricFog::GridConstants m_gridConstants{};
    VolumetricFog::MediumConstants m_mediumConstants{};
    VolumetricFog::LightingConstants m_lightingConstants{};
    VolumetricFog::IntegrationConstants m_integrationConstants{};

    D3D12_RESOURCE_STATES m_mediumVolumeState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES m_lightingVolumeState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES m_integratedVolumeState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_GPU_VIRTUAL_ADDRESS m_shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE m_cascadeShadowMapSRV{};

    float m_animationTime = 0.0f;
    uint32_t m_lastAnimationFrame = 0xFFFFFFFFu;

    bool m_hasShadowData = false;
    bool m_enabled = false;
};