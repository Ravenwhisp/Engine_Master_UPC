#pragma once

#include "IRenderPass.h"
#include "SimpleMath.h"

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Texture;
class DeferredShadingPass;

// builds per-tile point/spot light index lists (tiled deferred light culling)
class LightCullingPass final : public IRenderPass
{
public:
    struct TileCullingConstants
    {
        Matrix view = Matrix::Identity;

        float xScale = 1.0f;
        float yScale = 1.0f;
        float proj33 = 0.0f;
        float proj43 = 0.0f;

        uint32_t tileCountX = 0;
        uint32_t tileCountY = 0;
        uint32_t screenWidth = 0;
        uint32_t screenHeight = 0;
    };

public:
    LightCullingPass(ComPtr<ID3D12Device4> device, DeferredShadingPass* deferredShadingPass);
    ~LightCullingPass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    D3D12_GPU_VIRTUAL_ADDRESS getPointLightIndexBufferAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS getSpotLightIndexBufferAddress() const;

    // stays in sync with the buffer addresses above
    uint32_t getTileCountX() const { return m_constants.tileCountX; }
    uint32_t getTileCountY() const { return m_constants.tileCountY; }

private:
    static constexpr uint32_t TILE_SIZE = 8;
    static constexpr uint32_t MAX_LIGHTS_PER_TILE = 64;

    static uint32_t divideRoundUp(uint32_t value, uint32_t divisor);

    void createRootSignature();
    void createPipelineState();

    void ensureTileListBuffers(uint32_t requiredTileCount);

    void transitionBuffers(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);

private:
    ComPtr<ID3D12Device4> m_device;
    DeferredShadingPass* m_deferredShadingPass = nullptr;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    ComPtr<ID3D12Resource> m_pointLightIndexBuffer;
    ComPtr<ID3D12Resource> m_spotLightIndexBuffer;
    D3D12_RESOURCE_STATES m_bufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    // only grows, never shrinks - editor/game share this pass at different sizes,
    // so reallocating on every switch would free a buffer still in use elsewhere
    uint32_t m_allocatedTileCapacity = 0;

    Texture* m_inputDepthTexture = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS m_lightsCBAddress = 0;

    TileCullingConstants m_constants{};

    bool m_hasValidInput = false;
};
