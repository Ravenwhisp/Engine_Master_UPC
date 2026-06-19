#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <string>

using Microsoft::WRL::ComPtr;

struct RenderContext;
class Texture;

class PostProcessPass
{
public:
    explicit PostProcessPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx);

    void setBloomSource(D3D12_GPU_DESCRIPTOR_HANDLE bloomSrv, bool valid);

    void apply(ID3D12GraphicsCommandList4* commandList,
               D3D12_GPU_DESCRIPTOR_HANDLE sceneHDRSrv,
               D3D12_CPU_DESCRIPTOR_HANDLE targetRTV,
               const D3D12_VIEWPORT& viewport,
               const D3D12_RECT& scissorRect);

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

    std::shared_ptr<Texture> m_lutTexture;
    std::shared_ptr<Texture> m_identityLut;
    std::string              m_loadedLutPath;
    int                      m_lutSize = 2;

    std::shared_ptr<Texture> m_dummyTexture;

    D3D12_GPU_DESCRIPTOR_HANDLE m_bloomSrv{};
    bool                        m_bloomValid = false;
};
