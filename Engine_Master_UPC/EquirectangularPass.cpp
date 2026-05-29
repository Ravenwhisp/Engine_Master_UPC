#include "Globals.h"
#include "EquirectangularPass.h"

#include "ModuleDescriptors.h"
#include "SkyboxParams.h"
#include "Application.h"
#include "CommandQueue.h"
#include "ModuleD3D12.h"
#include "MD5.h"
#include "SkyBox.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include <d3dcompiler.h>
#include <PlatformHelpers.h>

EquirectangularPass::EquirectangularPass(ComPtr<ID3D12Device4> device)
{
    m_device = device;

    #if defined(_DEBUG)
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
        UINT compileFlags = 0;
    #endif



    ComPtr<ID3D12RootSignature> rootSignature;
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParameters[0].InitAsConstants(sizeof(SkyboxParams) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    ComPtr<ID3D12PipelineState> pso;
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"EquirectangularVertexShader.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"EquirectangularPixelShader.cso", &pixelShaderBlob));
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
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
    desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));
}

Texture* EquirectangularPass::renderCubemap(SkyBox* skybox)
{
    ModuleResources* moduleResources = app->getModuleResources();

    static const float PI = 3.14159265f;

    CommandQueue* commandQueue = app->getModuleD3D12()->getCommandQueue();
    ComPtr<ID3D12GraphicsCommandList4> commandList = commandQueue->getCommandList();

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    TextureDesc desc{};
    desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.width = static_cast<uint32_t>(2048);
    desc.height = static_cast<uint32_t>(2048);
    desc.arraySize = 6;
    desc.mipLevels = 8;
    desc.views = TextureView::RTV;
    desc.initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    
    Texture* skyboxTextureRtv = new Texture(hashToUID("SkyboxTextureRtv"), *m_device.Get(), desc);

    
    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);



    Matrix proj = Matrix::CreatePerspectiveFieldOfView(PI / 2.0f, 1.0f, 0.1f, 100.0f);

    Vector3 front[] = { Vector3(1,0,0), Vector3(-1,0,0), Vector3(0,1,0), Vector3(0,-1,0), Vector3(0,0,1), Vector3(0,0,-1) };
    Vector3 up[] = { Vector3(0,1,0), Vector3(0,1,0), Vector3(0,0,-1), Vector3(0,0,1), Vector3(0,1,0), Vector3(0,1,0) };

    //if (PIXIsAttachedForGpuCapture()) PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);

    for (size_t mip = 0; mip < desc.mipLevels; mip++)
    {
        //Viewport and scissor
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = desc.width >> mip;
        viewport.Height = desc.height >> mip;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissor = {};
        scissor.left = 0;
        scissor.top = 0;
        scissor.right = desc.width >> mip;
        scissor.bottom = desc.height >> mip;

        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);

        for (size_t i = 0; i < desc.arraySize; i++)
        {
            //BEGIN_EVENT(commandList.Get(), "Cubemap Generation");

            Matrix view = Matrix::CreateLookAt(Vector3::Zero, front[i], up[i]);
            Matrix viewProjection = view * proj;
            viewProjection = viewProjection.Transpose();

            SkyboxParams params{};
            params.vp = viewProjection;
            params.flipX = i <= 1 ? 0 : 1;
            params.flipZ = i <= 1 ? 1 : 0;

            D3D12_VERTEX_BUFFER_VIEW vertexBufferView = skybox->getVertexBuffer()->getVertexBufferView();
            D3D12_INDEX_BUFFER_VIEW  indexBufferView = skybox->getIndexBuffer()->getIndexBufferView();

            UINT subResourceIndex = D3D12CalcSubresource(mip, i, 0, desc.mipLevels, desc.arraySize);

            auto currentRtvHandle = skyboxTextureRtv->getContiguousRTV(mip* desc.arraySize + i).cpu;

            CD3DX12_RESOURCE_BARRIER barrierIn = CD3DX12_RESOURCE_BARRIER::Transition(skyboxTextureRtv->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, subResourceIndex);
            CD3DX12_RESOURCE_BARRIER barrierOut = CD3DX12_RESOURCE_BARRIER::Transition(skyboxTextureRtv->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, subResourceIndex);
    
            commandList->SetGraphicsRoot32BitConstants(0, sizeof(SkyboxParams) / sizeof(UINT32), &params, 0);
            commandList->SetGraphicsRootDescriptorTable(1, skybox->getHdrTexture()->getSRV().gpu);
            commandList->SetGraphicsRootDescriptorTable(2, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            commandList->IASetIndexBuffer(&indexBufferView);

            commandList->ResourceBarrier(1, &barrierIn);

            commandList->OMSetRenderTargets(1, &currentRtvHandle, 0, nullptr);
            float clearColor[4] = { 0, 0, 0, 1 };
            commandList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);
            commandList->DrawIndexedInstanced(static_cast<UINT>(skybox->getIndexBuffer()->getNumIndices()), 1, 0, 0, 0);
            commandList->ResourceBarrier(1, &barrierOut);

            //END_EVENT(commandList.Get());
        }
    }

    commandQueue->executeCommandList(commandList);
    //if (PIXIsAttachedForGpuCapture()) PIXEndCapture(TRUE);

    commandQueue->flush();

    auto finalTexture = new Texture(hashToUID("SkyboxTexture"), *m_device.Get(), skyboxTextureRtv->getD3D12Resource(), TextureView::SRV, DXGI_FORMAT_R16G16B16A16_FLOAT);

    delete skyboxTextureRtv;

    return finalTexture;
}
