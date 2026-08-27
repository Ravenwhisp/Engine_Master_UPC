#pragma once

#include "IRenderPass.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

class Texture;
class RenderSurface;
class VolumetricFogComputePass;

class VolumetricFogApplyPass final : public IRenderPass
{
public:
    VolumetricFogApplyPass(ComPtr<ID3D12Device4> device, VolumetricFogComputePass* computePass);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    struct ApplyConstants
    {
        float nearDistance = 0.1f;
        float maxDistance = 100.0f;
        float projectionA = 0.0f;
        float projectionB = 0.0f;

        uint32_t gridDepth = 64;
        uint32_t debugView = 0;
        float debugSlice = 0.5f;
        uint32_t padding = 0;
    };

    void createRootSignature();
    void createPipelineState();

    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    VolumetricFogComputePass* m_computePass = nullptr;
    RenderSurface* m_renderSurface = nullptr;
    Texture* m_integratedVolume = nullptr;
    Texture* m_depthTexture = nullptr;
    Texture* m_sceneHDR = nullptr;
    Texture* m_mediumVolume = nullptr;
    Texture* m_lightingVolume = nullptr;

    ApplyConstants m_constants{};
    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
    bool m_enabled = false;
};