#pragma once
#include "IRenderPass.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <string>

using Microsoft::WRL::ComPtr;

struct RenderContext;
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
        uint32_t pad0 = 0;
    };

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

    // Per-frame state captured in prepare().
    RenderSurface* m_surface = nullptr;
    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT     m_scissorRect{};
    bool           m_runBloom = false;
};
