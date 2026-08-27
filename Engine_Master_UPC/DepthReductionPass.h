#pragma once

#include "IRenderPass.h"

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Texture;

class DepthReductionPass final : public IRenderPass
{
public:
    struct ReductionConstants
    {
        uint32_t inputWidth = 0;
        uint32_t inputHeight = 0;
        uint32_t padding0 = 0;
        uint32_t padding1 = 0;
    };

public:
    explicit DepthReductionPass(ComPtr<ID3D12Device4> device);
    ~DepthReductionPass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    const Texture* getResultTexture() const
    {
        return m_resultTexture;
    }

    Texture* getResultTexture()
    {
        return m_resultTexture;
    }

private:
    static constexpr uint32_t TILE_SIZE = 8;

    static uint32_t divideRoundUp(uint32_t value, uint32_t divisor);

    void createRootSignature();
    void createPipelineStates();

    void ensureReductionTextures(
        uint32_t depthWidth,
        uint32_t depthHeight);

    void dispatchReductionStage(
        ID3D12GraphicsCommandList4* commandList,
        ID3D12PipelineState* pipelineState,
        Texture& inputTexture,
        Texture& outputTexture,
        uint32_t inputWidth,
        uint32_t inputHeight);

    void transitionTexture(
        ID3D12GraphicsCommandList4* commandList,
        Texture& texture,
        D3D12_RESOURCE_STATES beforeState,
        D3D12_RESOURCE_STATES afterState);

private:
    ComPtr<ID3D12Device4> m_device;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_initialReductionPipelineState;
    ComPtr<ID3D12PipelineState> m_reductionPipelineState;

    Texture* m_inputDepthTexture = nullptr;

    std::unique_ptr<Texture> m_pingTexture;
    std::unique_ptr<Texture> m_pongTexture;

    Texture* m_resultTexture = nullptr;

    uint32_t m_depthWidth = 0;
    uint32_t m_depthHeight = 0;

    uint32_t m_reductionTextureWidth = 0;
    uint32_t m_reductionTextureHeight = 0;
};
