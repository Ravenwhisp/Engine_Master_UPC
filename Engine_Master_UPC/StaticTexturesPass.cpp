#include "Globals.h"
#include "StaticTexturesPass.h"

#include "ModuleDescriptors.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ModuleD3D12.h"
#include "Texture.h"
#include "MD5.h"


#include <d3dcompiler.h>
#include <PlatformHelpers.h>

StaticTexturesPass::StaticTexturesPass(ComPtr<ID3D12Device4> device)
{
    m_device = device;

    #if defined(_DEBUG)
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
        UINT compileFlags = 0;
    #endif



    ComPtr<ID3D12RootSignature> rootSignature;
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));



    ComPtr<ID3D12PipelineState> pso;
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"BRDFVertexShader.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"BRDFPixelShader.cso", &pixelShaderBlob));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.InputLayout = { nullptr, 0 };
    desc.pRootSignature = m_rootSignature.Get();
    desc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    desc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.DepthStencilState.DepthEnable = FALSE;
    desc.DepthStencilState.StencilEnable = FALSE;
    desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    desc.SampleMask = UINT_MAX;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R16G16_FLOAT;
    desc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));
}

void StaticTexturesPass::apply()
{
    ModuleResources* moduleResources = app->getModuleResources();

    renderBrdf(moduleResources);
}

void StaticTexturesPass::renderBrdf(ModuleResources* moduleResources)
{
    CommandQueue* commandQueue = app->getModuleD3D12()->getCommandQueue();
    ComPtr<ID3D12GraphicsCommandList4> commandList = commandQueue->getCommandList();

    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (float)1024;
    viewport.Height = (float)1024;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = 1024;
    scissorRect.bottom = 1024;

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    
    TextureDesc desc{};
    desc.format = DXGI_FORMAT_R16G16_FLOAT;
    desc.width = static_cast<uint32_t>(1024);
    desc.height = static_cast<uint32_t>(1024);
    desc.views = TextureView::RTV;
    desc.initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    Texture* enviromentBrdfTextureRtv = new Texture(hashToUID("EnviromentBrdfTextureRtv"), *m_device.Get(), desc);
    auto rtv = enviromentBrdfTextureRtv->getRTV();
    
    CD3DX12_RESOURCE_BARRIER barrierIn = CD3DX12_RESOURCE_BARRIER::Transition(enviromentBrdfTextureRtv->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    CD3DX12_RESOURCE_BARRIER barrierOut = CD3DX12_RESOURCE_BARRIER::Transition(enviromentBrdfTextureRtv->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->ResourceBarrier(1, &barrierIn);

    commandList->OMSetRenderTargets(1, &rtv.cpu, FALSE, nullptr);
    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);
    
    commandList->ResourceBarrier(1, &barrierOut);

    commandQueue->executeCommandList(commandList);
    commandQueue->flush();

    auto environmentBrdfTexture = std::make_shared<Texture>(hashToUID("EnviromentBrdfTexture"), *m_device.Get(), enviromentBrdfTextureRtv->getD3D12Resource().Get(), TextureView::SRV, DXGI_FORMAT_R16G16_FLOAT);
    enviromentBrdfTextureRtv->setName(L"EnviromentBrdfTextureRtv");
    moduleResources->setEnvironmentBrdfTexture(environmentBrdfTexture);
}
