#include "Globals.h"
#include "RenderModule.h"

#include "Application.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "ResourcesModule.h"
#include "CameraModule.h"

#include "SceneModule.h"
#include "UIModule.h"

#include "RingBuffer.h"
#include "RenderTexture.h"

#include "LightComponent.h"
#include "Transform.h"

#include "Skybox.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "DepthBuffer.h"

#include "Logger.h"

#include "GameObject.h"
#include "CameraComponent.h"

bool RenderModule::init()
{
    m_settings = app->getSettings();
    auto d3d12 = app->getD3D12Module();
    auto device = d3d12->getDevice();

    m_ringBuffer = app->getResourcesModule()->createRingBuffer(10);

    m_skyBoxPass = new SkyBoxPass(device, app->getSceneModule()->getSkyboxSettings());
    m_meshRendererPass = new MeshRendererPass(device, m_ringBuffer);
    m_imGuiPass = new ImGuiPass(device, d3d12->getWindowHandle(), app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getCPUHandle(0), app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getGPUHandle(0));
    m_debugDrawPass = new DebugDrawPass(device, d3d12->getCommandQueue()->getD3D12CommandQueue().Get(), false);

    //m_skyboxTexture = app->getResourcesModule()->createTextureCubeFromFile(path(m_settings->skybox.path), "Skybox");
    //m_hasSkybox = (m_skyboxTexture != nullptr);

    m_editorScreenRT = app->getResourcesModule()->createRenderTexture(m_size.x, m_size.y);
    m_playScreenRT = app->getResourcesModule()->createRenderTexture(m_size.x, m_size.y);
    m_editorScreenDS = app->getResourcesModule()->createDepthBuffer(m_size.x, m_size.y);
	m_playScreenDS = app->getResourcesModule()->createDepthBuffer(m_size.x, m_size.y);

    return true;
}

bool RenderModule::postInit()
{

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

        m_editorScreenRT = app->getResourcesModule()->createRenderTexture(newSize.x, newSize.y);
        m_editorScreenRT->setName(L"editorScreenRT");
        m_editorScreenDS = app->getResourcesModule()->createDepthBuffer(newSize.x, newSize.y);
        m_editorScreenDS->setName(L"editorScreenDS");

        m_playScreenRT = app->getResourcesModule()->createRenderTexture(newSize.x, newSize.y);
        m_playScreenRT->setName(L"playScreenRT");
        m_playScreenDS = app->getResourcesModule()->createDepthBuffer(newSize.x, newSize.y);
        m_playScreenDS->setName(L"playScreenDS");
    }

    // Transition editor scene texture to render target
    transitionResource(m_commandList, m_editorScreenRT->getD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Transition play scene texture to render target
    transitionResource(m_commandList, m_playScreenRT->getD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Render the editor scene to texture
    renderEditorScene(m_commandList, m_editorScreenRT->getRTV(0).cpu, m_editorScreenDS->getDSV().cpu, m_size.x, m_size.y);
    // Render the play scene to texture
    renderPlayScene(m_commandList, m_playScreenRT->getRTV(0).cpu, m_playScreenDS->getDSV().cpu, m_size.x, m_size.y);

    // Transition back to shader resource state
    transitionResource(m_commandList, m_editorScreenRT->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    // Transition back to shader resource state
    transitionResource(m_commandList, m_playScreenRT->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    transitionResource(m_commandList, _swapChain->getCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderBackground(m_commandList, _swapChain->getCurrentRenderTargetView().cpu, _swapChain->getDepthStencilView(), _swapChain->getViewport(), _swapChain->getScissorRect());

    m_imGuiPass->startFrame();
    ImGuizmo::BeginFrame();
}


void RenderModule::renderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

void RenderModule::renderEditorScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    //This part since is common between SkyboxPass and MeshRendererPass it will belongs right not to the RenderModule
    D3D12_VIEWPORT viewport = D3D12_VIEWPORT{ 0.0, 0.0, float(m_size.x), float(m_size.y) , 0.0, 1.0 };
    D3D12_RECT scissorRect = D3D12_RECT{ 0, 0,  static_cast<LONG>(m_size.x),  static_cast<LONG>(m_size.y) };
    renderBackground(commandList, rtvHandle, dsvHandle, viewport, scissorRect);

    Matrix viewMatrix;
    Matrix projectionMatrix;
    Vector3 cameraPosition;

    if (app->getCurrentCameraPerspective())
    {
        const CameraComponent* camera = app->getCurrentCameraPerspective();
        viewMatrix = camera->getViewMatrix();
        projectionMatrix = camera->getProjectionMatrix();
        cameraPosition = camera->getOwner()->GetTransform()->getPosition();
    }
    else
    {
        viewMatrix = app->getCameraModule()->getView();
        projectionMatrix = app->getCameraModule()->getProjection();
        cameraPosition = app->getCameraModule()->getPosition();
    }

    m_skyBoxPass->setView(viewMatrix);
    m_skyBoxPass->setProjection(projectionMatrix);
    m_skyBoxPass->apply(commandList);


    m_meshRendererPass->setCameraPosition(cameraPosition);
    m_meshRendererPass->setView(viewMatrix);
    m_meshRendererPass->setProjection(projectionMatrix);

    /*m_meshRendererPass->setRenderTargetView(m_screenRT->getRTV(0).cpu);
    m_meshRendererPass->setDepthStencilView(m_screenDS->getDSV().cpu);

    m_meshRendererPass->setViewport(_swapChain->getViewport());
    m_meshRendererPass->setRectScissor(_swapChain->getScissorRect());*/

    // NOT IDEAL TO CALL HERE THIS RENDER FUCNTION, THAT IS NOT DOING ANYTHING RELATED TO RENDER ANYMORE
    app->getSceneModule()->render(commandList);
    m_meshRendererPass->setMeshes(app->getSceneModule()->getAllMeshRenderers());
    m_meshRendererPass->apply(commandList);

    // REPEATED CODE WITH MESH RENDERER PASS
    m_debugDrawPass->setView(viewMatrix);
    m_debugDrawPass->setProjection(projectionMatrix);
    m_debugDrawPass->setViewport(viewport);

    // THIS IS NOT THE IDEAL BUT IS TO MAKE SURE THAT ALL WORKS
    app->getEditorModule()->getSceneEditor()->renderDebugDrawPass(commandList);
    m_debugDrawPass->apply(commandList);

    app->getUIModule()->renderUI(commandList, viewport);
}

void RenderModule::renderPlayScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    if (app->getSceneModule()->getDefaultCamera())
    {
        //This part since is common between SkyboxPass and MeshRendererPass it will belongs right not to the RenderModule
        D3D12_VIEWPORT viewport = D3D12_VIEWPORT{ 0.0, 0.0, float(m_size.x), float(m_size.y) , 0.0, 1.0 };
        D3D12_RECT scissorRect = D3D12_RECT{ 0, 0,  static_cast<LONG>(m_size.x),  static_cast<LONG>(m_size.y) };
        renderBackground(commandList, rtvHandle, dsvHandle, viewport, scissorRect);

        Matrix viewMatrix;
        Matrix projectionMatrix;
        Vector3 cameraPosition;

        const CameraComponent* camera = app->getSceneModule()->getDefaultCamera();
        viewMatrix = camera->getViewMatrix();
        projectionMatrix = camera->getProjectionMatrix();
        cameraPosition = camera->getOwner()->GetTransform()->getPosition();

        m_skyBoxPass->setView(viewMatrix);
        m_skyBoxPass->setProjection(projectionMatrix);
        m_skyBoxPass->apply(commandList);

        m_meshRendererPass->setCameraPosition(cameraPosition);
        m_meshRendererPass->setView(viewMatrix);
        m_meshRendererPass->setProjection(projectionMatrix);

        /*m_meshRendererPass->setRenderTargetView(m_screenRT->getRTV(0).cpu);
        m_meshRendererPass->setDepthStencilView(m_screenDS->getDSV().cpu);

        m_meshRendererPass->setViewport(_swapChain->getViewport());
        m_meshRendererPass->setRectScissor(_swapChain->getScissorRect());*/

        // NOT IDEAL TO CALL HERE THIS RENDER FUCNTION, THAT IS NOT DOING ANYTHING RELATED TO RENDER ANYMORE
        app->getSceneModule()->render(commandList);
        m_meshRendererPass->setMeshes(app->getSceneModule()->getAllMeshRenderers());
        m_meshRendererPass->apply(commandList);

        app->getUIModule()->renderUI(commandList, viewport);
    }
}

void RenderModule::render()
{
    auto _commandList = app->getD3D12Module()->getCommandList();
    auto _swapChain = app->getD3D12Module()->getSwapChain();

    m_imGuiPass->apply(_commandList);

    transitionResource(_commandList, _swapChain->getCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

bool RenderModule::cleanUp()
{
    m_editorScreenRT.reset();
    m_editorScreenDS.reset();

    m_playScreenRT.reset();
    m_playScreenDS.reset();

    delete m_skyBoxPass;
    m_skyBoxPass = nullptr;

    delete m_meshRendererPass;
    m_meshRendererPass = nullptr;

    delete m_imGuiPass;
    m_imGuiPass = nullptr;

    delete m_debugDrawPass;
    m_debugDrawPass = nullptr;

    delete m_ringBuffer;
    m_ringBuffer = nullptr;

    return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderModule::getGPUEditorScreenRT()
{
    return m_editorScreenRT->getSRV().gpu;
}
D3D12_GPU_DESCRIPTOR_HANDLE RenderModule::getGPUPlayScreenRT()
{
    return m_playScreenRT->getSRV().gpu;
}

D3D12_GPU_VIRTUAL_ADDRESS RenderModule::allocateInRingBuffer(const void* data, size_t size)
{
    return m_ringBuffer->allocate(data, size, app->getD3D12Module()->getCurrentFrame());
}

bool RenderModule::applySkyboxSettings(const SkyboxSettings& settings)
{
    m_skyBoxPass->setSettings(settings);

    return true;
}

void RenderModule::transitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}