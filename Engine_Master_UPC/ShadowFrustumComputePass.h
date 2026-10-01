#pragma once

#include "IRenderPass.h"
#include "IDebugDrawable.h"
#include "SimpleMath.h"
#include "ShadowTypes.h"

#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <array>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class DepthReductionPass;
class LightComponent;

class ShadowFrustumComputePass final : public IRenderPass, public IDebugDrawable
{
public:
    struct FrustumConstants
    {
        Matrix inverseView = Matrix::Identity;
        Matrix view = Matrix::Identity;
        Matrix projection = Matrix::Identity;

        Vector3 lightDirection = Vector3::Zero;
        float sunDistance = 20.0f;

        float shadowBias = 0.0005f;
        float shadowStrength = 1.0f;
        uint32_t pcfEnabled = 0;
        uint32_t pcfRadius = 0;

        float shadowMapTexelSize = 0.0f;
        uint32_t cascadeCount = 1;
        uint32_t cascadeFitMode = static_cast<uint32_t>(ShadowCascadeFitMode::FIT_TO_CASCADE);
        uint32_t shadowLightIndex = 0;

        float cascadeSplit0 = 0.10f;
        float cascadeSplit1 = 0.30f;
        float cascadeSplit2 = 0.60f;
        uint32_t cascadeDebugEnabled = 0;

        uint32_t cascadeUpdateMask = 0xFu;
        uint32_t cascadeUpdatePadding0 = 0;
        uint32_t cascadeUpdatePadding1 = 0;
        uint32_t cascadeUpdatePadding2 = 0;

    };

private:
    struct DebugCaptureMetadata
    {
        Matrix view = Matrix::Identity;
        Matrix projection = Matrix::Identity;
        bool valid = false;
    };

public:
    ShadowFrustumComputePass(ComPtr<ID3D12Device4> device, DepthReductionPass* depthReductionPass);

    ~ShadowFrustumComputePass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    void debugDraw() override;

    uint32_t getCascadeUpdateMask() const { return m_cascadeUpdateMask; }

    D3D12_GPU_VIRTUAL_ADDRESS getShadowDataBufferAddress() const;

    bool isEnabled() const
    {
        return m_enabled;
    }

    bool hasValidResult() const
    {
        return m_hasValidResult;
    }

private:
    static constexpr float SHADOW_MIN_ORTHO_SIZE = 10.0f;
    static constexpr float SHADOW_LIGHT_DISTANCE_PADDING = 20.0f;
    static constexpr uint32_t SHADOW_VIEW_COUNT = 2;

private:
    void createRootSignature();
    void createPipelineState();
    void createOutputBuffer();

    void createDebugReadbackBuffers();
    void refreshDebugReadbackForCurrentFrame();
    void recordDebugReadback(ID3D12GraphicsCommandList4* commandList);

    const LightComponent* findMainShadowCastingDirectionalLight() const;

    void transitionOutputBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState);

private:
    ComPtr<ID3D12Device4> m_device;

    DepthReductionPass* m_depthReductionPass = nullptr;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::array<ComPtr<ID3D12Resource>, SHADOW_VIEW_COUNT> m_shadowDataBuffers;
    std::array<D3D12_RESOURCE_STATES, SHADOW_VIEW_COUNT> m_outputBufferStates{};

    FrustumConstants m_constants{};
    D3D12_GPU_VIRTUAL_ADDRESS m_constantsAddress = 0;

    bool m_enabled = false;
    bool m_hasValidResult = false;

    std::vector<ComPtr<ID3D12Resource>> m_debugReadbackBuffers;
    std::vector<bool> m_debugReadbackPending;
    std::vector<DebugCaptureMetadata> m_debugCaptureMetadata;

    ShadowDataCB m_debugShadowData{};

    Matrix m_debugCameraView = Matrix::Identity;
    Matrix m_debugCameraProjection = Matrix::Identity;

    bool m_hasDebugShadowData = false;
    bool m_captureDebugReadback = false;
    bool m_drawDebugForCurrentView = false;

    uint32_t m_observedDebugFrameIndex = 0;
    uint64_t m_observedDebugFrameFenceValue = 0;
    bool m_hasObservedDebugFrame = false;

    struct CascadeUpdateHistory
    {
        bool valid = false;
        uint32_t shadowMapSize = 0;
        uint32_t cascadeCount = 0;
        uint32_t cascadeFitMode = UINT32_MAX;
        float cascadeSplit0 = -1.0f;
        float cascadeSplit1 = -1.0f;
        float cascadeSplit2 = -1.0f;
    };

    std::array<CascadeUpdateHistory, SHADOW_VIEW_COUNT> m_cascadeUpdateHistory{};
    uint32_t m_cascadeUpdateMask = 0xFu;
    uint32_t m_currentShadowView = 0;


};