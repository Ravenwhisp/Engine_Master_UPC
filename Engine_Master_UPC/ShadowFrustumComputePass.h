#pragma once

#include "IRenderPass.h"
#include "SimpleMath.h"

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class DepthReductionPass;
class LightComponent;

class ShadowFrustumComputePass final : public IRenderPass
{
public:
    struct FrustumConstants
    {
        Matrix inverseView = Matrix::Identity;
        Matrix projection = Matrix::Identity;

        Vector3 lightDirection = Vector3::Zero;
        float sunDistance = 20.0f;

        float minOrthoSize = 10.0f;
        Vector3 padding = Vector3::Zero;
    };

public:
    ShadowFrustumComputePass(
        ComPtr<ID3D12Device4> device,
        DepthReductionPass* depthReductionPass);

    ~ShadowFrustumComputePass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    D3D12_GPU_VIRTUAL_ADDRESS getLightViewProjectionBufferAddress() const;

    bool isEnabled() const
    {
        return m_enabled;
    }

private:
    static constexpr float SHADOW_MIN_ORTHO_SIZE = 10.0f;
    static constexpr float SHADOW_LIGHT_DISTANCE_PADDING = 20.0f;

private:
    void createRootSignature();
    void createPipelineState();
    void createOutputBuffer();

    const LightComponent* findMainShadowCastingDirectionalLight() const;

    void transitionOutputBuffer(
        ID3D12GraphicsCommandList4* commandList,
        D3D12_RESOURCE_STATES newState);

private:
    ComPtr<ID3D12Device4> m_device;

    DepthReductionPass* m_depthReductionPass = nullptr;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    ComPtr<ID3D12Resource> m_lightViewProjectionBuffer;

    D3D12_RESOURCE_STATES m_outputBufferState =
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    FrustumConstants m_constants{};

    bool m_enabled = false;
};