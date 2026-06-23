#pragma once
#include "IRenderPass.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <string>

using Microsoft::WRL::ComPtr;

struct RenderContext;
struct PostProcessSettings;
class RenderSurface;
class Texture;
class BloomPass;

class PostProcessPass : public IRenderPass
{
public:
    explicit PostProcessPass(ComPtr<ID3D12Device4> device);
    ~PostProcessPass() override;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    struct PostProcessParams
    {
        float exposure = 0.0f;
        float bloomIntensity = 0.5f;
        float lutSize = 2.0f;
        float caStrength = 1.0f;

        uint32_t enableBloom = 0;
        uint32_t enableLUT = 0;
        uint32_t enableCA = 0;
        uint32_t enableHeartbeat = 0;

        // heartbeat outputs
        float    hbHealthVignette = 0.0f;
        float    hbSepVignette = 0.0f;
        float    hbPulse = 0.0f;
        uint32_t hbPulseIsLub = 1;

        // heartbeat outputs
        float    hbCrit = 0.0f;
        float    hbDesat = 0.0f;
        float    hbSwayX = 0.0f;
        float    hbSwayY = 0.0f;

        //death fade outputs
        float    deathDesat = 0.0f;  // 0..1 desaturation towards grey
        float    deathFade = 0.0f;   // 0..1 fade towards black
        float    deathBlur = 0.0f;   // 0..1 out-of-focus blur amount
        float    deathPad0 = 0.0f;

        //outline
        uint32_t enableOutline = 0;
        float    outlineThickness = 1.5f;
        float    outlineThreshold = 0.02f;
        float    outlineIntensity = 0.6f;

        // outline
        float    outlineColorR = 0.05f;
        float    outlineColorG = 0.04f;
        float    outlineColorB = 0.05f;
        float    outlineWobble = 1.0f;

        //outline
        float    outlineNoiseScale = 90.0f;
        float    outlineBreakup = 0.5f;
        float    outlinePad0 = 0.0f;
        float    outlinePad1 = 0.0f;
    };

    void updateHeartbeat(const PostProcessSettings& settings, const RenderContext& ctx, float dt);

    void updateDeathFade(const PostProcessSettings& settings, float dt);

    ComPtr<ID3D12Device4>       m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    PostProcessParams m_params;

    std::unique_ptr<BloomPass> m_bloomPass;

    // Colour-grading LUT and a neutral identity LUT fallback.
    std::shared_ptr<Texture> m_lutTexture;
    std::shared_ptr<Texture> m_identityLut;
    std::string              m_loadedLutPath;
    int                      m_lutSize = 2;

    // 1x1 placeholder bound to the bloom slot when bloom is inactive.
    std::shared_ptr<Texture> m_dummyTexture;

    RenderSurface* m_surface = nullptr;
    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT     m_scissorRect{};
    bool           m_runBloom = false;

    uint32_t m_lastFrame = 0xFFFFFFFFu;

    float    m_hbDubTimer = -1.0f;
    float    m_hbLubTimer = -1.0f;
    float    m_hbPulseAnim = 0.0f;
    int      m_hbPulseType = 0;      
    float    m_hbSwayAngle = 0.0f;

    float    m_deathTime = 0.0f;
};
