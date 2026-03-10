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

    m_activeScene = app->getSceneModule();

    return true;
}

bool RenderModule::postInit()
{

    return true;
}

void RenderModule::renderToTexture(ID3D12GraphicsCommandList4* commandList, RenderTexture* rt, DepthBuffer* ds, std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE)> renderFunc)
{
    transitionResource(commandList, rt->getD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderFunc(rt->getRTV(0).cpu, ds->getDSV().cpu);
    transitionResource(commandList, rt->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void RenderModule::preRender()
{
    m_ringBuffer->free(app->getD3D12Module()->getLastCompletedFrame());

    auto commandList = app->getD3D12Module()->getCommandList();
    auto swapChain = app->getD3D12Module()->getSwapChain();

#ifdef GAME_RELEASE
    transitionResource(commandList, swapChain->getCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderGameToBackbuffer(commandList, swapChain->getCurrentRenderTargetView().cpu, swapChain->getDepthStencilView(), swapChain->getViewport(), swapChain->getScissorRect());
#else
    auto newSize = app->getEditorModule()->getSceneEditorSize();

    if (m_size.x != newSize.x || m_size.y != newSize.y)
    {
        app->getD3D12Module()->getCommandQueue()->flush();
        m_size = newSize;

        m_editorScreenRT.reset();
        m_editorScreenRT = app->getResourcesModule()->createRenderTexture(newSize.x, newSize.y);
        m_editorScreenRT->setName(L"editorScreenRT");
        m_editorScreenDS.reset();
        m_editorScreenDS = app->getResourcesModule()->createDepthBuffer(newSize.x, newSize.y);
        m_editorScreenDS->setName(L"editorScreenDS");

        m_playScreenRT.reset();
        m_playScreenRT = app->getResourcesModule()->createRenderTexture(newSize.x, newSize.y);
        m_playScreenRT->setName(L"playScreenRT");
        m_playScreenDS.reset();
        m_playScreenDS = app->getResourcesModule()->createDepthBuffer(newSize.x, newSize.y);
        m_playScreenDS->setName(L"playScreenDS");
    }

    renderToTexture(commandList, m_editorScreenRT.get(), m_editorScreenDS.get(),
        [&](auto rtv, auto dsv)
        {
            renderEditorScene(commandList, rtv, dsv, m_size.x, m_size.y);
        });

    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        renderToTexture(commandList, m_playScreenRT.get(), m_playScreenDS.get(),
            [&](auto rtv, auto dsv)
            {
                renderPlayScene(commandList, rtv, dsv, m_size.x, m_size.y);
            });
    }

    transitionResource(commandList, swapChain->getCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderBackground(commandList, swapChain->getCurrentRenderTargetView().cpu, swapChain->getDepthStencilView(), swapChain->getViewport(), swapChain->getScissorRect());
#endif

    m_imGuiPass->startFrame();
    ImGuizmo::BeginFrame();
}

RenderModule::RenderCamera RenderModule::getEditorCamera()
{
    RenderCamera camera;

    if (app->getCurrentCameraPerspective())
    {
        const CameraComponent* c = app->getCurrentCameraPerspective();

        camera.view = c->getViewMatrix();
        camera.projection = c->getProjectionMatrix();
        camera.position = c->getOwner()->GetTransform()->getPosition();
    }
    else
    {
        camera.view = app->getCameraModule()->getView();
        camera.projection = app->getCameraModule()->getProjection();
        camera.position = app->getCameraModule()->getPosition();
    }

    return camera;
}

RenderModule::RenderCamera RenderModule::getGameCamera()
{
    RenderCamera camera;

    const CameraComponent* c = app->getSceneModule()->getDefaultCamera();

    if (!c)
    {
        return camera;
    }

    camera.view = c->getViewMatrix();
    camera.projection = c->getProjectionMatrix();
    camera.position = c->getOwner()->GetTransform()->getPosition();
    camera.valid = true;

    return camera;
}

void RenderModule::renderScene(ID3D12GraphicsCommandList4* commandList, const RenderCamera& camera, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, bool renderDebug)
{
    renderBackground(commandList, rtvHandle, dsvHandle, viewport, scissorRect);

    SceneModule* scene = m_activeScene ? m_activeScene : app->getSceneModule();

    if (scene == app->getSceneModule())
    {
        m_skyBoxPass->setView(camera.view);
        m_skyBoxPass->setProjection(camera.projection);
        m_skyBoxPass->apply(commandList);
    }

    m_meshRendererPass->setCameraPosition(camera.position);
    m_meshRendererPass->setView(camera.view);
    m_meshRendererPass->setProjection(camera.projection);

    scene->render(commandList);

    const std::vector<MeshRenderer*>& meshes = scene->getAllMeshRenderers();
    m_meshRendererPass->setMeshes(meshes);
    m_meshRendererPass->apply(commandList);

    //Debug
    if (renderDebug)
    {
        m_debugDrawPass->setView(camera.view);
        m_debugDrawPass->setProjection(camera.projection);
        m_debugDrawPass->setViewport(viewport);

        app->getEditorModule()->getSceneEditor()->renderDebugDrawPass(commandList);
        m_debugDrawPass->apply(commandList);
    }

    app->getUIModule()->renderUI(commandList, viewport);
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
    D3D12_VIEWPORT viewport = { 0,0,width,height,0,1 };
    D3D12_RECT scissorRect = { 0,0,(LONG)width,(LONG)height };

    RenderCamera camera = getEditorCamera();

    renderScene(commandList, camera, rtvHandle, dsvHandle, viewport, scissorRect, true);
}

void RenderModule::renderPlayScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    RenderCamera camera = getGameCamera();

    if (!camera.valid)
    {
        return;
    }

    D3D12_VIEWPORT viewport = { 0,0,width,height,0,1 };
    D3D12_RECT scissorRect = { 0,0,(LONG)width,(LONG)height };

    renderScene(commandList, camera, rtvHandle, dsvHandle, viewport, scissorRect, false);
}

void RenderModule::renderGameToBackbuffer(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
    RenderCamera camera = getGameCamera();

    if (!camera.valid)
    {
        return;
    }

    renderScene(commandList, camera, rtvHandle, dsvHandle, viewport, scissorRect, false);
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