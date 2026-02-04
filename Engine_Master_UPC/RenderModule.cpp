#include "Globals.h"
#include "RenderModule.h"

#include "Application.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "ResourcesModule.h"
#include "CameraModule.h"

#include "RingBuffer.h"
#include "Scene.h"
#include "RenderTexture.h"


bool RenderModule::init()
{
    return true;
}

bool RenderModule::postInit()
{
    m_rootSignature = app->getD3D12Module()->createRootSignature();
    m_pipelineState = app->getD3D12Module()->createPipelineStateObject(m_rootSignature.Get());

    m_screenRT = app->getResourcesModule()->createRenderTexture(m_size.x, m_size.y);
    m_screenDS = app->getResourcesModule()->createDepthBuffer(m_size.x, m_size.y);
    app->getCameraModule()->setAspectRatio(static_cast<float>(m_size.x), static_cast<float>(m_size.y));

    m_scene = new Emeika::Scene();
    m_ringBuffer = app->getResourcesModule()->createRingBuffer(10);
    return true;
}

void RenderModule::preRender()
{
    m_ringBuffer->free(app->getD3D12Module()->getLastCompletedFrame());

    auto m_commandList = app->getD3D12Module()->getCommandList();
    auto _swapChain = app->getD3D12Module()->getSwapChain();

    auto newSize = app->getEditorModule()->getSceneEditorSize();

    if (m_size.x != newSize.x || m_size.y != newSize.y) 
    {
        app->getD3D12Module()->getCommandQueue()->flush();
        m_size = newSize;
        m_screenRT = app->getResourcesModule()->createRenderTexture(newSize.x, newSize.y);
        m_screenRT->setName(L"ScreenRT");
        m_screenDS = app->getResourcesModule()->createDepthBuffer(newSize.x, newSize.y);
        m_screenDS->setName(L"ScreenDS");
        app->getCameraModule()->resize(newSize.x, newSize.y);
    }

    // Transition scene texture to render target
    transitionResource(m_commandList, m_screenRT->getD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Render the scene to texture
    renderScene(m_commandList, m_screenRT->getRTV(0).cpu, m_screenDS->getDSV().cpu, m_size.x, m_size.y);

    // Transition back to shader resource state
    transitionResource(m_commandList, m_screenRT->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    transitionResource(m_commandList, _swapChain->getCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderBackground(m_commandList, _swapChain->getCurrentRenderTargetView().cpu, _swapChain->getDepthStencilView(), _swapChain->getViewport().Width, _swapChain->getViewport().Height);
}

void RenderModule::render()
{
    auto m_commandList = app->getD3D12Module()->getCommandList();
    auto _swapChain = app->getD3D12Module()->getSwapChain();

    transitionResource(m_commandList, _swapChain->getCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

bool RenderModule::cleanUp()
{
    m_screenRT.reset();
    m_screenDS.reset();

    delete m_scene;
    delete m_ringBuffer;

    return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderModule::getGPUScreenRT()
{
    return m_screenRT->getSRV().gpu;
}

D3D12_GPU_VIRTUAL_ADDRESS RenderModule::allocateInRingBuffer(const void* data, size_t size)
{
    return m_ringBuffer->allocate(data, size, app->getD3D12Module()->getCurrentFrame());
}

void RenderModule::transitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}

void RenderModule::renderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
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

void RenderModule::renderScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    // Clear + draw
    renderBackground(commandList, rtvHandle, dsvHandle, width, height);

    // Bind root signature (must be set before any draw calls)
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    //Set input assembler
    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    SceneData& sceneData = m_scene->getData();
    sceneData.view = app->getCameraModule()->getPosition();

    commandList->SetGraphicsRootConstantBufferView(1, m_ringBuffer->allocate(&sceneData, sizeof(SceneData), app->getD3D12Module()->getCurrentFrame()));
    commandList->SetGraphicsRootDescriptorTable(4, app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(m_sampleType));

    m_scene->render(commandList, app->getCameraModule()->getViewMatrix(), app->getCameraModule()->getProjectionMatrix());

    //DebugDrawPass
    app->getEditorModule()->getSceneEditor()->renderDebugDrawPass(commandList);

}