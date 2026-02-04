#include "Globals.h"
#include "RenderModule.h"

#include "Application.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "ResourcesModule.h"
#include "CameraModule.h"

#include "Resources.h"

#include "RingBuffer.h"
#include "Scene.h"

bool RenderModule::init()
{
    return true;
}

bool RenderModule::postInit()
{
    m_rootSignature = app->GetD3D12Module()->CreateRootSignature();
    m_pipelineState = app->GetD3D12Module()->CreatePipelineStateObject(m_rootSignature.Get());

    screenRT = app->GetResourcesModule()->CreateRenderTexture(size.x, size.y);
    screenDS = app->GetResourcesModule()->CreateDepthBuffer(size.x, size.y);
    app->GetCameraModule()->SetAspectRatio(static_cast<float>(size.x), static_cast<float>(size.y));

    scene = new Emeika::Scene();
    ringBuffer = app->GetResourcesModule()->CreateRingBuffer(10);
    return true;
}

void RenderModule::preRender()
{
    ringBuffer->Free(app->GetD3D12Module()->GetLastCompletedFrame());

    auto m_commandList = app->GetD3D12Module()->GetCommandList();
    auto _swapChain = app->GetD3D12Module()->GetSwapChain();

    auto newSize = app->GetEditorModule()->GetSceneEditorSize();
    if (size.x != newSize.x || size.y != newSize.y) {
        app->GetD3D12Module()->GetCommandQueue()->Flush();
        size = newSize;
        screenRT = app->GetResourcesModule()->CreateRenderTexture(newSize.x, newSize.y);
        screenRT->SetName(L"ScreenRT");
        screenDS = app->GetResourcesModule()->CreateDepthBuffer(newSize.x, newSize.y);
        screenDS->SetName(L"ScreenDS");
        app->GetCameraModule()->Resize(newSize.x, newSize.y);
    }

    // Transition scene texture to render target
    TransitionResource(m_commandList, screenRT->GetD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Render the scene to texture
    RenderScene(m_commandList, screenRT->RTV(0).cpu, screenDS->DSV().cpu, size.x, size.y);

    // Transition back to shader resource state
    TransitionResource(m_commandList, screenRT->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    TransitionResource(m_commandList, _swapChain->GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    RenderBackground(m_commandList, _swapChain->GetCurrentRenderTargetView().cpu, _swapChain->GetDepthStencilView(), _swapChain->GetViewport().Width, _swapChain->GetViewport().Height);
}

void RenderModule::render()
{
    auto m_commandList = app->GetD3D12Module()->GetCommandList();
    auto _swapChain = app->GetD3D12Module()->GetSwapChain();

    TransitionResource(m_commandList, _swapChain->GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

bool RenderModule::cleanUp()
{
    screenRT.reset();
    screenDS.reset();

    delete scene;
    delete ringBuffer;

    return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderModule::GetGPUScreenRT()
{
    return screenRT->SRV().gpu;
}

D3D12_GPU_VIRTUAL_ADDRESS RenderModule::AllocateInRingBuffer(const void* data, size_t size)
{
    return ringBuffer->Allocate(data, size, app->GetD3D12Module()->GetCurrentFrame());
}

void RenderModule::TransitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}

void RenderModule::RenderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Setup viewport & scissor
    D3D12_VIEWPORT offscreenViewport = { 0,0, width, height, 0.0f, 1.0f };
    D3D12_RECT offscreenScissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

    commandList->RSSetViewports(1, &offscreenViewport);
    commandList->RSSetScissorRects(1, &offscreenScissorRect);
}

void RenderModule::RenderScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    // Clear + draw
    RenderBackground(commandList, rtvHandle, dsvHandle, width, height);

    // Bind root signature (must be set before any draw calls)
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    //Set input assembler
    ID3D12DescriptorHeap* descriptorHeaps[] = { app->GetDescriptorsModule()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetHeap(), app->GetDescriptorsModule()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).GetHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    SceneData& sceneData = scene->GetData();
    sceneData.view = app->GetCameraModule()->GetPosition();

    commandList->SetGraphicsRootConstantBufferView(1, ringBuffer->Allocate(&sceneData, sizeof(SceneData), app->GetD3D12Module()->GetCurrentFrame()));
    commandList->SetGraphicsRootDescriptorTable(4, app->GetDescriptorsModule()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).GetGPUHandle(_sampleType));

    //Scene models
    scene->Render(commandList, app->GetCameraModule()->GetViewMatrix(), app->GetCameraModule()->GetProjectionMatrix());

    //DebugDrawPass
    app->GetEditorModule()->GetSceneEditor()->RenderDebugDrawPass(commandList);

}