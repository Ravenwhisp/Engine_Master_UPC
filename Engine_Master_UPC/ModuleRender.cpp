#include "Globals.h"
#include "ModuleRender.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleEditor.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include "ModuleGameView.h"
#include "ModuleScene.h"
#include "ModuleNavigation.h"
#include "ModuleUI.h"

#include "RingBuffer.h"
#include "RenderSurface.h"
#include "Texture.h"

#include "Scene.h"
#include "GameObject.h"
#include "CameraComponent.h"
#include "Transform.h"
#include "MeshRenderer.h"

#include "ImGuiPass.h"
#include "SkyBoxPass.h"
#include "MeshRendererPass.h"
#include "DebugDrawPass.h"
#include "UIImagePass.h"
#include "FontPass.h"
#include "SkinningComputePass.h"
#include "Quadtree.h"
#include "RenderContext.h"

std::unique_ptr<RenderSurface> ModuleRender::createSurface(float width, float height)
{
    auto surface = std::make_unique<RenderSurface>();

    auto colorTex = std::shared_ptr<Texture>(app->getModuleResources()->createRenderTexture(width, height));

    auto depthTex = std::shared_ptr<Texture>(app->getModuleResources()->createDepthBuffer(width, height));

    surface->attachTexture(RenderSurface::COLOR_0, colorTex);
    surface->attachTexture(RenderSurface::DEPTH_STENCIL, depthTex);

    return surface;
}

void ModuleRender::resizeSurface(RenderSurface& surface, float width, float height)
{
    surface.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}


bool ModuleRender::init()
{
    m_settings = app->getSettings();
    m_moduleGameView = app->getModuleGameView();

    auto  d3d12 = app->getModuleD3D12();
    auto* device = d3d12->getDevice();

    m_ringBuffer = app->getModuleResources()->createRingBuffer(10);

    // Build the ordered render-pass list.
    auto debugDrawPass = std::make_unique<DebugDrawPass>(  device, d3d12->getCommandQueue()->getD3D12CommandQueue().Get(),/*useMSAA=*/false);

    debugDrawPass->registerStatic(app->getModuleNavigation());
    debugDrawPass->registerStatic(app->getModuleEditor()->getWindowSceneEditor());

    m_renderPasses.push_back(std::make_unique<SkyBoxPass>( device, app->getModuleScene()->getScene()->getSkyBoxSettings()));
    m_renderPasses.push_back(std::make_unique<MeshRendererPass>(device));
    m_renderPasses.push_back(std::make_unique<SkinningComputePass>(device));   //  <-------------- CRASH HERE
    m_renderPasses.push_back(std::move(debugDrawPass));
    m_renderPasses.push_back(std::make_unique<UIImagePass>(device));
    m_renderPasses.push_back(std::make_unique<FontPass>(device));

    // ImGui lives outside the pass list because startFrame() / apply() must
    // bracket the entire editor render, not just the scene render.
    m_imGuiPass = std::make_unique<ImGuiPass>(
        device,
        d3d12->getWindowHandle(),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getCPUHandle(0),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getGPUHandle(0));

    // Create initial render surfaces at the default viewport size.
    m_editorSurface = createSurface(m_size.x, m_size.y);
    m_playSurface = createSurface(m_size.x, m_size.y);

    return true;
}

void ModuleRender::preRender()
{
    m_ringBuffer->free(app->getModuleD3D12()->getLastCompletedFrame());

    auto* commandList = app->getModuleD3D12()->getCommandList();
    auto* swapChain = app->getModuleD3D12()->getSwapChain();

#ifdef GAME_RELEASE
    transitionResource(commandList,
        swapChain->getCurrentRenderTarget(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderGameToBackbuffer(commandList,
        swapChain->getCurrentRenderTargetView().cpu,
        swapChain->getDepthStencilView(),
        swapChain->getViewport(),
        swapChain->getScissorRect());
#else
    const ImVec2 newSize = app->getModuleEditor()->getWindowSceneEditorSize();

    // Resize both surfaces lazily when the viewport size changes.
    if (m_size.x != newSize.x || m_size.y != newSize.y)
    {
        app->getModuleD3D12()->getCommandQueue()->flush();
        m_size = newSize;

        resizeSurface(*m_editorSurface, newSize.x, newSize.y);
        resizeSurface(*m_playSurface, newSize.x, newSize.y);
    }

    // Editor viewport
    renderToSurface(commandList, *m_editorSurface,
        [&](D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv)
        {
            renderEditorScene(commandList, rtv, dsv, m_size.x, m_size.y);
        });

    // Play viewport (only when the engine is in play mode)
    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        renderToSurface(commandList, *m_playSurface,
            [&](D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv)
            {
                renderPlayScene(commandList, rtv, dsv, m_size.x, m_size.y);
            });
    }

    transitionResource(commandList, swapChain->getCurrentRenderTarget()->getD3D12Resource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderBackground(commandList, swapChain->getRenderSurface().getTexture(RenderSurface::COLOR_0)->getRTV().cpu, swapChain->getRenderSurface().getTexture(RenderSurface::DEPTH_STENCIL)->getDSV().cpu, swapChain->getViewport(), swapChain->getScissorRect());
#endif

    m_imGuiPass->startFrame();
    ImGuizmo::BeginFrame();
}

void ModuleRender::render()
{
    auto* commandList = app->getModuleD3D12()->getCommandList();
    auto* swapChain = app->getModuleD3D12()->getSwapChain();

    m_imGuiPass->apply(commandList);

    transitionResource(commandList, swapChain->getCurrentRenderTarget()->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

bool ModuleRender::cleanUp()
{
    m_renderPasses.clear();
    m_imGuiPass.reset();

    m_editorSurface.reset();
    m_playSurface.reset();

    delete m_ringBuffer;
    m_ringBuffer = nullptr;

    return true;
}


D3D12_GPU_DESCRIPTOR_HANDLE ModuleRender::getGPUEditorScreenRT()
{
    return m_editorSurface->getTexture(RenderSurface::COLOR_0)->getSRV().gpu;
}

D3D12_GPU_DESCRIPTOR_HANDLE ModuleRender::getGPUPlayScreenRT()
{
    return m_playSurface->getTexture(RenderSurface::COLOR_0)->getSRV().gpu;
}

D3D12_GPU_VIRTUAL_ADDRESS ModuleRender::allocateInRingBuffer(const void* data, size_t size)
{
    return m_ringBuffer->allocate(data, size, app->getModuleD3D12()->getCurrentFrame());
}

void ModuleRender::renderToSurface(ID3D12GraphicsCommandList4* commandList,RenderSurface& surface,std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE)> renderFunc)
{
    auto colorTex = surface.getTexture(RenderSurface::COLOR_0);
    auto depthTex = surface.getTexture(RenderSurface::DEPTH_STENCIL);

    transitionResource(commandList,colorTex->getD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderFunc(colorTex->getRTV(0).cpu, depthTex->getDSV().cpu);

    transitionResource(commandList, colorTex->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}


ModuleRender::RenderCamera ModuleRender::getEditorCamera()
{
    RenderCamera camera;

    if (const CameraComponent* c = app->getCurrentCameraPerspective())
    {
        camera.view = c->getViewMatrix();
        camera.projection = c->getProjectionMatrix();
        camera.position = c->getOwner()->GetTransform()->getPosition();
        camera.valid = true;
    }
    else
    {
        camera.view = app->getModuleCamera()->getView();
        camera.projection = app->getModuleCamera()->getProjection();
        camera.position = app->getModuleCamera()->getPosition();
        camera.valid = true;
    }

    return camera;
}

ModuleRender::RenderCamera ModuleRender::getGameCamera()
{
    RenderCamera camera;

    const CameraComponent* c = app->getModuleScene()->getScene()->getDefaultCamera();
    if (!c) return camera;

    camera.view = c->getViewMatrix();
    camera.projection = c->getProjectionMatrix();
    camera.position = c->getOwner()->GetTransform()->getPosition();
    camera.valid = true;

    return camera;
}

void ModuleRender::renderScene( ID3D12GraphicsCommandList4* commandList, const RenderCamera& camera, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,D3D12_VIEWPORT viewport, D3D12_RECT  scissorRect, bool renderDebug)
{
    renderBackground(commandList, rtvHandle, dsvHandle, viewport, scissorRect);

    RenderContext ctx{
        .view = camera.view,
        .projection = camera.projection,
        .cameraPosition = camera.position,
        .viewport = viewport,
        .scissorRect = scissorRect,
        .ringBuffer = m_ringBuffer,
        .renderDebug = renderDebug,
        .uiTextCommands = &app->getModuleUI()->getTextCommands(),
        .uiImageCommands = &app->getModuleUI()->getImageCommands(),
        .skyBoxSettings = &app->getModuleScene()->getScene()->getSkyBoxSettings(),
    };

    for (auto& pass : m_renderPasses)
    {
        pass->prepare(ctx);
    }


    for (auto& pass : m_renderPasses)
    {
        pass->apply(commandList);
    }
}

void ModuleRender::renderBackground( ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

void ModuleRender::renderEditorScene( ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    D3D12_VIEWPORT viewport = { 0, 0, width, height, 0, 1 };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

    renderScene(commandList, getEditorCamera(), rtvHandle, dsvHandle, viewport, scissorRect, /*debug=*/true);
}

void ModuleRender::renderPlayScene( ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    const RenderCamera camera = getGameCamera();
    if (!camera.valid) return;

    D3D12_VIEWPORT viewport = { 0, 0, width, height, 0, 1 };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

    renderScene(commandList, camera, rtvHandle, dsvHandle, viewport, scissorRect, m_moduleGameView->getShowDebugWindow());
}

void ModuleRender::renderGameToBackbuffer( ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
    const RenderCamera camera = getGameCamera();
    if (!camera.valid) return;

    renderScene(commandList, camera, rtvHandle, dsvHandle, viewport, scissorRect, m_moduleGameView->getShowDebugWindow());
}

void ModuleRender::transitionResource( ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES  beforeState,D3D12_RESOURCE_STATES  afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}