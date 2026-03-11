#include "Globals.h"
#include "ModuleRender.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleEditor.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include "ModuleGameView.h"

#include "ModuleScene.h"
#include "ModuleUI.h"

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

bool ModuleRender::init()
{
    m_settings = app->getSettings();
    m_gameViewModule = app->getModuleGameView();

    auto d3d12 = app->getModuleD3D12();
    auto device = d3d12->getDevice();

    m_ringBuffer = app->getModuleResources()->createRingBuffer(10);

    m_skyBoxPass = new SkyBoxPass(device, app->getModuleScene()->getSkyboxSettings());
    m_meshRendererPass = new MeshRendererPass(device, m_ringBuffer);
    m_imGuiPass = new ImGuiPass(device, d3d12->getWindowHandle(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getCPUHandle(0), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getGPUHandle(0));
    m_debugDrawPass = new DebugDrawPass(device, d3d12->getCommandQueue()->getD3D12CommandQueue().Get(), false);

    //m_skyboxTexture = app->getModuleResources()->createTextureCubeFromFile(path(m_settings->skybox.path), "Skybox");
    //m_hasSkybox = (m_skyboxTexture != nullptr);

    m_editorScreenRT = app->getModuleResources()->createRenderTexture(m_size.x, m_size.y);
    m_playScreenRT = app->getModuleResources()->createRenderTexture(m_size.x, m_size.y);
    m_editorScreenDS = app->getModuleResources()->createDepthBuffer(m_size.x, m_size.y);
	m_playScreenDS = app->getModuleResources()->createDepthBuffer(m_size.x, m_size.y);

    return true;
}

void ModuleRender::renderToTexture(ID3D12GraphicsCommandList4* commandList, RenderTexture* rt, DepthBuffer* ds, std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE)> renderFunc)
{
    transitionResource(commandList, rt->getD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderFunc(rt->getRTV(0).cpu, ds->getDSV().cpu);
    transitionResource(commandList, rt->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void ModuleRender::preRender()
{
    m_ringBuffer->free(app->getModuleD3D12()->getLastCompletedFrame());

    auto commandList = app->getModuleD3D12()->getCommandList();
    auto swapChain = app->getModuleD3D12()->getSwapChain();

#ifdef GAME_RELEASE
    transitionResource(commandList, swapChain->getCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderGameToBackbuffer(commandList, swapChain->getCurrentRenderTargetView().cpu, swapChain->getDepthStencilView(), swapChain->getViewport(), swapChain->getScissorRect());
#else
    auto newSize = app->getModuleEditor()->getSceneEditorSize();

    if (m_size.x != newSize.x || m_size.y != newSize.y)
    {
        app->getModuleD3D12()->getCommandQueue()->flush();
        m_size = newSize;

        m_editorScreenRT.reset();
        m_editorScreenRT = app->getModuleResources()->createRenderTexture(newSize.x, newSize.y);
        m_editorScreenRT->setName(L"editorScreenRT");
        m_editorScreenDS.reset();
        m_editorScreenDS = app->getModuleResources()->createDepthBuffer(newSize.x, newSize.y);
        m_editorScreenDS->setName(L"editorScreenDS");

        m_playScreenRT.reset();
        m_playScreenRT = app->getModuleResources()->createRenderTexture(newSize.x, newSize.y);
        m_playScreenRT->setName(L"playScreenRT");
        m_playScreenDS.reset();
        m_playScreenDS = app->getModuleResources()->createDepthBuffer(newSize.x, newSize.y);
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

ModuleRender::RenderCamera ModuleRender::getEditorCamera()
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
        camera.view = app->getModuleCamera()->getView();
        camera.projection = app->getModuleCamera()->getProjection();
        camera.position = app->getModuleCamera()->getPosition();
    }

    return camera;
}

ModuleRender::RenderCamera ModuleRender::getGameCamera()
{
    RenderCamera camera;

    const CameraComponent* c = app->getModuleScene()->getDefaultCamera();

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

void ModuleRender::renderScene(ID3D12GraphicsCommandList4* commandList, const RenderCamera& camera, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, bool renderDebug)
{
    renderBackground(commandList, rtvHandle, dsvHandle, viewport, scissorRect);

    m_skyBoxPass->setView(camera.view);
    m_skyBoxPass->setProjection(camera.projection);
    m_skyBoxPass->apply(commandList);

    m_meshRendererPass->setCameraPosition(camera.position);
    m_meshRendererPass->setView(camera.view);
    m_meshRendererPass->setProjection(camera.projection);

    ModuleScene* scene = app->getModuleScene();
    scene->render(commandList);

    const std::vector<MeshRenderer*>& meshes = scene->getAllMeshRenderers();
    m_triangles = 0;
    for (MeshRenderer* mesh : meshes)
    {
        m_triangles += mesh->getTriangles();
    }

    m_meshRendererPass->setMeshes(meshes);
    m_meshRendererPass->apply(commandList);

    //Debug
    if (renderDebug)
    {
        m_debugDrawPass->setView(camera.view);
        m_debugDrawPass->setProjection(camera.projection);
        m_debugDrawPass->setViewport(viewport);

        app->getModuleEditor()->getSceneEditor()->renderDebugDrawPass(commandList);
        m_debugDrawPass->apply(commandList);
    }

    app->getModuleUI()->renderUI(commandList, viewport);
}

void ModuleRender::renderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

void ModuleRender::renderEditorScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    D3D12_VIEWPORT viewport = { 0,0,width,height,0,1 };
    D3D12_RECT scissorRect = { 0,0,(LONG)width,(LONG)height };

    RenderCamera camera = getEditorCamera();

    renderScene(commandList, camera, rtvHandle, dsvHandle, viewport, scissorRect, true);
}

void ModuleRender::renderPlayScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    RenderCamera camera = getGameCamera();

    if (!camera.valid)
    {
        return;
    }

    D3D12_VIEWPORT viewport = { 0,0,width,height,0,1 };
    D3D12_RECT scissorRect = { 0,0,(LONG)width,(LONG)height };

    renderScene(commandList, camera, rtvHandle, dsvHandle, viewport, scissorRect, m_gameViewModule->getShowDebugWindow());
}

void ModuleRender::renderGameToBackbuffer(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
    RenderCamera camera = getGameCamera();

    if (!camera.valid)
    {
        return;
    }

    renderScene(commandList, camera, rtvHandle, dsvHandle, viewport, scissorRect, m_gameViewModule->getShowDebugWindow());
}

void ModuleRender::render()
{
    auto _commandList = app->getModuleD3D12()->getCommandList();
    auto _swapChain = app->getModuleD3D12()->getSwapChain();

    m_imGuiPass->apply(_commandList);

    transitionResource(_commandList, _swapChain->getCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

bool ModuleRender::cleanUp()
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

D3D12_GPU_DESCRIPTOR_HANDLE ModuleRender::getGPUEditorScreenRT()
{
    return m_editorScreenRT->getSRV().gpu;
}
D3D12_GPU_DESCRIPTOR_HANDLE ModuleRender::getGPUPlayScreenRT()
{
    return m_playScreenRT->getSRV().gpu;
}

D3D12_GPU_VIRTUAL_ADDRESS ModuleRender::allocateInRingBuffer(const void* data, size_t size)
{
    return m_ringBuffer->allocate(data, size, app->getModuleD3D12()->getCurrentFrame());
}

bool ModuleRender::applySkyboxSettings(const SkyboxSettings& settings)
{
    m_skyBoxPass->setSettings(settings);

    return true;
}

void ModuleRender::transitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}