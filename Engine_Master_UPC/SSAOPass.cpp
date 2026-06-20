#include "Globals.h"
#include "SSAOPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "RingBuffer.h"
#include "RenderContext.h"
#include "Texture.h"

#include "PlatformHelpers.h"
#include "OptickProfiler.h"

#include <d3dcompiler.h>
#include <random>
#include <algorithm>


SSAOPass::SSAOPass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    createKernel();

    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};

    CD3DX12_DESCRIPTOR_RANGE depthRange;
    depthRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE normalRange;
    normalRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &depthRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &normalRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC pointClampSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(
        _countof(rootParameters),
        rootParameters,
        1,
        &pointClampSampler,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error));

    DXCall(m_device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"FullscreenTriangleVS.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"SSAO.cso", &pixelShaderBlob));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8_UNORM;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&m_pipelineState)));
}

void SSAOPass::prepare(const RenderContext& ctx)
{
    PERF_RENDER("SSAOPass::prepare");

    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    m_outputTexture = ctx.ssaoRawTexture;
    m_depthTexture = ctx.ssaoDepthTexture;
    m_normalTexture = ctx.ssaoNormalTexture;

    uploadConstants(ctx);
}

void SSAOPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    PERF_RENDER("SSAOPass::apply");

    if (!m_outputTexture || !m_depthTexture || !m_normalTexture || m_ssaoCBAddress == 0)
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barrierToWrite =
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_outputTexture->getD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

    commandList->ResourceBarrier(1, &barrierToWrite);

    const float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    commandList->ClearRenderTargetView(
        m_outputTexture->getRTV().cpu,
        clearColor,
        0,
        nullptr);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_outputTexture->getRTV().cpu;

    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootConstantBufferView(0, m_ssaoCBAddress);
    commandList->SetGraphicsRootDescriptorTable(1, m_depthTexture->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(2, m_normalTexture->getSRV().gpu);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    CD3DX12_RESOURCE_BARRIER barrierToRead =
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_outputTexture->getD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    commandList->ResourceBarrier(1, &barrierToRead);
}

void SSAOPass::createKernel()
{
    std::uniform_real_distribution<float> randoms(0.0f, 1.0f);
    std::default_random_engine generator(0);

    constexpr float radius = 0.5f;

    for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i)
    {
        DirectX::SimpleMath::Vector3 sample(
            randoms(generator) * 2.0f - 1.0f,
            randoms(generator) * 2.0f - 1.0f,
            randoms(generator));

        sample.Normalize();

        sample *= randoms(generator);

        float scale = static_cast<float>(i) / static_cast<float>(SSAO_KERNEL_SIZE);
        scale = 0.1f + scale * scale * (1.0f - 0.1f);

        sample *= scale * radius;

        m_kernel[i] = DirectX::SimpleMath::Vector4(sample.x, sample.y, sample.z, 0.0f);
    }
}

void SSAOPass::uploadConstants(const RenderContext& ctx)
{
    if (!ctx.ringBuffer)
    {
        m_ssaoCBAddress = 0;
        return;
    }

    m_ssaoData.projection = ctx.projection.Transpose();
    m_ssaoData.inverseProjection = ctx.projection.Invert().Transpose();

    for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i)
    {
        m_ssaoData.samples[i] = m_kernel[i];
    }

    const float width = std::max(1.0f, ctx.viewport.Width);
    const float height = std::max(1.0f, ctx.viewport.Height);

    m_ssaoData.params = DirectX::SimpleMath::Vector4(
        0.5f,                               // radius
        0.025f,                             // bias
        1.0f,                               // strength
        static_cast<float>(SSAO_KERNEL_SIZE));

    m_ssaoData.screenParams = DirectX::SimpleMath::Vector4(
        width,
        height,
        1.0f / width,
        1.0f / height);

    m_ssaoData.frameParams = DirectX::SimpleMath::Vector4(
        static_cast<float>(app->getModuleD3D12()->getCurrentFrame()),
        0.0f,
        0.0f,
        0.0f);

    m_ssaoCBAddress = ctx.ringBuffer->allocate(
        &m_ssaoData,
        sizeof(SSAODataCB),
        app->getModuleD3D12()->getCurrentFrame());
}