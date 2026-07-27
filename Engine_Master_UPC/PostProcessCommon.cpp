#include "Globals.h"
#include "PostProcessCommon.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <PlatformHelpers.h>

namespace PostProcess
{
    D3D12_STATIC_SAMPLER_DESC bilinearClampSampler(UINT shaderRegister)
    {
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = shaderRegister;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        return sampler;
    }

    ComPtr<ID3D12PipelineState> createFullscreenPSO(ID3D12Device4* device, ID3D12RootSignature* rootSignature, const wchar_t* pixelShaderCso, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat, bool additiveBlend)
    {
        ComPtr<ID3DBlob> vertexShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(L"BRDFVertexShader.cso", &vertexShaderBlob));

        ComPtr<ID3DBlob> pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(pixelShaderCso, &pixelShaderBlob));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
        desc.InputLayout = { nullptr, 0 };
        desc.pRootSignature = rootSignature;
        desc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
        desc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        if (additiveBlend)
        {
            desc.BlendState.RenderTarget[0].BlendEnable = TRUE;
            desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
            desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
            desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        }
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        desc.DepthStencilState.DepthEnable = FALSE;
        desc.DepthStencilState.StencilEnable = FALSE;
        desc.DSVFormat = dsvFormat;
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = rtvFormat;
        desc.SampleDesc = { 1, 0 };

        ComPtr<ID3D12PipelineState> pso;
        DXCall(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));
        return pso;
    }

    void drawFullscreenTriangle(ID3D12GraphicsCommandList* commandList)
    {
        commandList->IASetVertexBuffers(0, 0, nullptr);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);
    }
}
