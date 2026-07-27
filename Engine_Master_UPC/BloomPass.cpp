#include "Globals.h"
#include "BloomPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "PostProcessSettings.h"
#include "PostProcessCommon.h"
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

    D3D12_STATIC_SAMPLER_DESC sampler = PostProcess::bilinearClampSampler();

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
    // Bloom chain renders into HDR targets; no depth.
    return PostProcess::createFullscreenPSO(m_device.Get(), m_rootSignature.Get(), pixelShaderCso, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN, additiveBlend);
}

void BloomPass::prepare(const RenderContext& ctx)
{
    (void)ctx;

    if (Scene* scene = app->getModuleScene()->getScene())
    {
        const PostProcessSettings& pp = scene->getPostProcessSettings();
        m_params.threshold = pp.bloomThreshold;
        // Keep a hard ceiling so the fp16 bloom chain can never reach Inf.
        m_params.maxBrightness = std::min(std::max(pp.bloomClamp, 0.0f), 8192.0f);
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

    PostProcess::drawFullscreenTriangle(commandList);

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
