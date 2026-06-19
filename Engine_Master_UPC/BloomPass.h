#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

struct RenderContext;
class Texture;

class BloomPass
{
public:
    explicit BloomPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx);
    void apply(ID3D12GraphicsCommandList4* commandList, D3D12_GPU_DESCRIPTOR_HANDLE sceneHDRSrv);

    D3D12_GPU_DESCRIPTOR_HANDLE getBloomSRV() const;

private:
    static constexpr int      LEVELS = 5;
    static constexpr uint32_t BASE_WIDTH = 640;
    static constexpr uint32_t BASE_HEIGHT = 360;

    struct BloomParams
    {
        float threshold = 1.0f;
        float pad0 = 0.0f;
        float pad1 = 0.0f;
        float pad2 = 0.0f;
    };

    ComPtr<ID3D12PipelineState> buildPSO(const wchar_t* pixelShaderCso, bool additiveBlend);
    void renderLevel(ID3D12GraphicsCommandList4* commandList, ID3D12PipelineState* pso, D3D12_GPU_DESCRIPTOR_HANDLE inputSrv, int level);

    ComPtr<ID3D12Device4>       m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_thresholdPSO;
    ComPtr<ID3D12PipelineState> m_downsamplePSO;
    ComPtr<ID3D12PipelineState> m_upsamplePSO;

    std::shared_ptr<Texture> m_chain[LEVELS];
    uint32_t                 m_width[LEVELS]{};
    uint32_t                 m_height[LEVELS]{};

    BloomParams m_params;
};
