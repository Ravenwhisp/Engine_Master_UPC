#include "Globals.h"
#include "OutlinePass.h"

#include "ModuleDescriptors.h"
#include "RenderContext.h"
#include "SceneDataCB.h"
#include "Texture.h"
#include "Application.h"
#include "RingBuffer.h"

#include "PlatformHelpers.h"
#include "OptickProfiler.h"

#include <d3dcompiler.h>

OutlinePass::OutlinePass(ComPtr<ID3D12Device4> device)
{
    m_device = device;
    m_sceneDataCB = std::make_unique<SceneDataCB>();

    createRootSignature();
    createPipelineState();
}

void OutlinePass::prepare(const RenderContext& ctx)
{
    PERF_RENDER("OutlinePass::prepare");

    {
        PERF_RENDER("DeferredShadingPass::prepare::SetupCamera");
        m_view = &ctx.view;
        m_projection = &ctx.projection;
        m_sceneDataCB->viewPos = ctx.cameraPosition;
    }

    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    m_depthTexture = ctx.outlineDepthTexture;
    m_normalTexture = ctx.outlineNormalTexture;

    PERF_RENDER("DeferredShadingPass::prepare::UploadSceneDataCB");
    m_sceneDataCBAddress = ctx.ringBuffer->allocate(m_sceneDataCB.get(), sizeof(SceneDataCB));
}

void OutlinePass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "OutlinePass");

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2];

    rtvs[0] = m_depthTexture->getRTV().cpu;
    rtvs[1] = m_normalTexture->getRTV().cpu;

    commandList->OMSetRenderTargets(2, rtvs, FALSE, nullptr);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootConstantBufferView(0, m_sceneDataCBAddress);

}

void OutlinePass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParams[3] = {};

    CD3DX12_DESCRIPTOR_RANGE normalRange, sampleRange;
    normalRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
    sampleRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); //Model view projection
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); //Scene data
    rootParams[2].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_ALL); //Model data

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature, error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void OutlinePass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"OutlineVS.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"OutlinePS.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 2;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.RTVFormats[1] = DXGI_FORMAT_R32_FLOAT;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}
