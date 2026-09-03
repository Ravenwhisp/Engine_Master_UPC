#pragma once

#include "IRenderPass.h"

class ID3D12GraphicsCommandList4;
class LightComponent;
class SceneLightingSettings;
class GPULightsConstantBuffer;
class GPUSpotLight;

struct LightCullingCB 
{
    Matrix view;
    Matrix projection;
    Matrix inverseProjection;
    uint32_t screenWidth;
    uint32_t screenHeight;
    uint32_t padding[2];
};

class LightCullingPass : public IRenderPass
{
public:
    LightCullingPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;
    void resize(uint32_t width, uint32_t height);

    const D3D12_GPU_VIRTUAL_ADDRESS getLightIndexesAddress() const { return m_lightIndexesBufferAddress; }

private:
    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>		m_rootSignature;
    ComPtr<ID3D12PipelineState>		m_pipelineState;

    D3D12_GPU_VIRTUAL_ADDRESS m_lightsAddress;

    D3D12_GPU_VIRTUAL_ADDRESS m_lightIndexesBufferAddress;
    uint32_t m_tilesX = 0;
    uint32_t m_tilesY = 0;
    uint32_t m_lightIndexCapacity = 0;
    // Size of tile, in pixels (nxn)
    unsigned int m_tileSize = 8;

    std::unique_ptr<SceneLightingSettings> m_lighting;


    D3D12_GPU_VIRTUAL_ADDRESS m_lightCullingCBAddress{};

    GPULightsConstantBuffer packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const;
    Vector4 calculateBoundingSphere(GPUSpotLight& l) const;
};