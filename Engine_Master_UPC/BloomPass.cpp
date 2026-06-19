#include "Globals.h"
#include "BloomPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "PostProcessSettings.h"
#include "Texture.h"
#include "UID.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <PlatformHelpers.h>
#include <algorithm>

BloomPass::BloomPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].InitAsConstants(sizeof(BloomParams) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    m_thresholdPSO = buildPSO(L"BloomThresholdPixelShader.cso", /*additiveBlend=*/false);
    m_downsamplePSO = buildPSO(L"BloomDownsamplePixelShader.cso", /*additiveBlend=*/false);
    m_upsamplePSO = buildPSO(L"BloomUpsamplePixelShader.cso", /*additiveBlend=*/true);

    for (int i = 0; i < LEVELS; ++i)
    {
        m_width[i] = std::max(1u, BASE_WIDTH >> i);
        m_height[i] = std::max(1u, BASE_HEIGHT >> i);

        TextureDesc desc{};
        desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.width = m_width[i];
        desc.height = m_height[i];
        desc.mipLevels = 1;
        desc.views = TextureView::SRV | TextureView::RTV;
        desc.initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        desc.shaderVisibleSRV = true;

        m_chain[i] = std::make_shared<Texture>(GenerateUID(), *m_device.Get(), desc);
        m_chain[i]->setName(L"BloomChain");
    }
}

ComPtr<ID3D12PipelineState> BloomPass::buildPSO(const wchar_t* pixelShaderCso, bool additiveBlend)
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"BRDFVertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(pixelShaderCso, &pixelShaderBlob));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.InputLayout = { nullptr, 0 };
    desc.pRootSignature = m_rootSignature.Get();
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
    desc.SampleMask = UINT_MAX;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.SampleDesc = { 1, 0 };

    ComPtr<ID3D12PipelineState> pso;
    DXCall(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));
    return pso;
}

void BloomPass::prepare(const RenderContext& ctx)
{
    (void)ctx;

    if (Scene* scene = app->getModuleScene()->getScene())
    {
        m_params.threshold = scene->getPostProcessSettings().bloomThreshold;
    }
}

void BloomPass::renderLevel(ID3D12GraphicsCommandList4* commandList, ID3D12PipelineState* pso, D3D12_GPU_DESCRIPTOR_HANDLE inputSrv, int level)
{
    Texture* tex = m_chain[level].get();

    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_width[level]), static_cast<float>(m_height[level]), 0.0f, 1.0f };
    D3D12_RECT     scissor = { 0, 0, static_cast<LONG>(m_width[level]), static_cast<LONG>(m_height[level]) };

    CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(tex->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &toRT);

    commandList->SetPipelineState(pso);
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = tex->getRTV(0).cpu;
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(BloomParams) / sizeof(UINT32), &m_params, 0);
    commandList->SetGraphicsRootDescriptorTable(1, inputSrv);

    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(tex->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toSRV);
}

void BloomPass::apply(ID3D12GraphicsCommandList4* commandList, D3D12_GPU_DESCRIPTOR_HANDLE sceneHDRSrv)
{
    // Bright-pass: HDR scene -> chain[0].
    renderLevel(commandList, m_thresholdPSO.Get(), sceneHDRSrv, 0);

    // Downsample down the chain.
    for (int i = 1; i < LEVELS; ++i)
    {
        renderLevel(commandList, m_downsamplePSO.Get(), m_chain[i - 1]->getSRV().gpu, i);
    }

    for (int i = LEVELS - 2; i >= 0; --i)
    {
        renderLevel(commandList, m_upsamplePSO.Get(), m_chain[i + 1]->getSRV().gpu, i);
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE BloomPass::getBloomSRV() const
{
    return m_chain[0]->getSRV().gpu;
}
