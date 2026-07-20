#pragma once
#include <d3d12.h>
#include <dxgiformat.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace PostProcess
{
    D3D12_STATIC_SAMPLER_DESC bilinearClampSampler(UINT shaderRegister = 0);

    ComPtr<ID3D12PipelineState> createFullscreenPSO(
        ID3D12Device4* device,
        ID3D12RootSignature* rootSignature,
        const wchar_t* pixelShaderCso,
        DXGI_FORMAT rtvFormat,
        DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN,
        bool additiveBlend = false);

    void drawFullscreenTriangle(ID3D12GraphicsCommandList* commandList);
}
