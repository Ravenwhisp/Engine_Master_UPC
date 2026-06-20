#include "Globals.h"
#include "ModuleRender.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleEditor.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include "ModuleGameView.h"
#include "ModuleScene.h"
#include "ModuleParticleSystem.h"

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
#include "ParticlesPass.h"
#include "DebugDrawPass.h"
#include "UIImagePass.h"
#include "FontPass.h"
#include "StaticTexturesPass.h"
#include "SkinningComputePass.h"
#include "ShadowMapPass.h"
#include "SSAOGeometryPass.h"
#include "SSAOPass.h"
#include "Quadtree.h"
#include "RenderContext.h"
#include "WindowSceneEditor.h"

#include "OptickProfiler.h"

#pragma region GameLoop
bool ModuleRender::init()
{
    m_settings = app->getSettings();
    m_moduleGameView = app->getModuleGameView();

    auto  d3d12 = app->getModuleD3D12();
    auto* device = d3d12->getDevice();

    m_ringBuffer = app->getModuleResources()->createRingBuffer(30);

    // Build the one time render-passes.
    auto staticTexturesPass = new StaticTexturesPass(device);
    
    staticTexturesPass->apply();

    delete staticTexturesPass;

    // Build the ordered render-pass list.
    auto debugDrawPass = std::make_unique<DebugDrawPass>(device, d3d12->getCommandQueue()->getD3D12CommandQueue().Get(),/*useMSAA=*/false);

    m_debugDrawPass = debugDrawPass.get();
    debugDrawPass->registerStatic(app->getModuleNavigation());
    debugDrawPass->registerStatic(app->getModuleEditor()->getWindowSceneEditor());

    m_skinningComputePass = std::make_unique<SkinningComputePass>(device);
    m_shadowMapPass = std::make_unique<ShadowMapPass>(device);
    m_ssaoGeometryPass = std::make_unique<SSAOGeometryPass>(device);
    m_ssaoPass = std::make_unique<SSAOPass>(device);

    m_meshRenderPass = new MeshRendererPass (device);
    auto skyBoxPass = std::make_unique<SkyBoxPass>(device, app->getModuleScene()->getScene()->getSkyBoxSettings());
    m_skyBoxPass = skyBoxPass.get();

    m_renderPasses.push_back(std::move(skyBoxPass));
    m_renderPasses.push_back(std::unique_ptr<MeshRendererPass>(m_meshRenderPass));
    m_renderPasses.push_back(std::make_unique<ParticlesPass>(device));
    m_renderPasses.push_back(std::move(debugDrawPass));
    m_renderPasses.push_back(std::make_unique<UIImagePass>(device));
    m_renderPasses.push_back(std::make_unique<FontPass>(device));

    // ImGui lives outside the pass list because startFrame() / apply() must
    // bracket the entire editor render, not just the scene render.
    m_imGuiPass = std::make_unique<ImGuiPass>(device, d3d12->getWindowHandle(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getCPUHandle(0), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getGPUHandle(0));

    return true;
}

void ModuleRender::preRender()
{
    PERF_RENDER("ModuleRender::preRender");

    m_shadowMapRenderedThisFrame = false;
    m_currentShadowData = nullptr;

    if (m_pendingStopSimulation)
    {
        app->getModuleD3D12()->getCommandQueue()->flush();
        app->getModuleGameView()->stopGameSimulation();
        m_pendingStopSimulation = false;
    }

    m_ringBuffer->free(app->getModuleD3D12()->getLastCompletedFrame());

    auto* commandList = app->getModuleD3D12()->getCommandList();
    auto* swapChain = app->getModuleD3D12()->getSwapChain();

#ifndef GAME_RELEASE
    {
        m_shadowMapRenderedThisFrame = false;
        m_currentShadowData = nullptr;

        auto* commandList = app->getModuleD3D12()->getCommandList();

        PERF_RENDER("ModuleRender::RenderViewports");
        for (const ViewportEntry& entry : m_viewports)
        {
            renderToSurface(commandList, *entry.surface, [&](RenderSurface& surface, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv)
                {
                    if (entry.type == ViewportType::EDITOR)
                    {
                        PERF_RENDER("ModuleRender::RenderEditorScene");
                        renderEditorScene(commandList, &surface, rtv, dsv, entry.width, entry.height);
                    }
                    else
                    {
                        PERF_RENDER("ModuleRender::RenderPlayScene");
                        renderPlayScene(commandList, &surface, rtv, dsv, entry.width, entry.height);
                    }
                });
        }
    }
#endif
    app->getModuleD3D12()->executeCurrentCommandList();
    m_imGuiPass->startFrame();
}


void ModuleRender::render()
{
    auto* commandList = app->getModuleD3D12()->getCommandList();
    auto* swapChain = app->getModuleD3D12()->getSwapChain();

    transitionResource(commandList,
        swapChain->getRenderSurface().getTexture(RenderSurface::COLOR_0)->getD3D12Resource(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

#ifndef GAME_RELEASE

    renderGameToBackbuffer(commandList,
        &swapChain->getRenderSurface(),
        swapChain->getRenderSurface().getTexture(RenderSurface::COLOR_0)->getRTV().cpu,
        swapChain->getRenderSurface().getTexture(RenderSurface::DEPTH_STENCIL)->getDSV().cpu,
        swapChain->getViewport(),
        swapChain->getScissorRect());

#else

    renderGameToBackbuffer(commandList,
        swapChain->getRenderSurface().getTexture(RenderSurface::COLOR_0)->getRTV().cpu,
        swapChain->getRenderSurface().getTexture(RenderSurface::DEPTH_STENCIL)->getDSV().cpu,
        swapChain->getViewport(),
        swapChain->getScissorRect());

#endif

    m_imGuiPass->apply(commandList);

    transitionResource(commandList, swapChain->getRenderSurface().getTexture(RenderSurface::COLOR_0)->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

bool ModuleRender::cleanUp()
{
    if (app != nullptr &&
        app->getModuleD3D12() != nullptr &&
        app->getModuleD3D12()->getCommandQueue() != nullptr)
    {
        app->getModuleD3D12()->getCommandQueue()->flush();
    }

    m_ssaoPass.reset();
    m_ssaoGeometryPass.reset();
    m_shadowMapPass.reset();
    m_skinningComputePass.reset();

    m_renderPasses.clear();

    m_meshRenderPass = nullptr;
    m_debugDrawPass = nullptr;
    m_skyBoxPass = nullptr;

    m_imGuiPass.reset();

    delete m_ringBuffer;
    m_ringBuffer = nullptr;

    return true;
}
#pragma endregion



void ModuleRender::registerViewport(RenderSurface* surface, ViewportType type, float width, float height)
{
    if (!surface || width <= 0.0f || height <= 0.0f)
    {
        return;
    }

    uint32_t w = static_cast<uint32_t>(width);
    uint32_t h = static_cast<uint32_t>(height);

    for (ViewportEntry& entry : m_viewports)
    {
        if (entry.surface == surface) 
        {
            if (entry.width != w || entry.height != h)
            {
                entry.width = w;
                entry.height = h;
                surface->resize(w, h);
                app->getModuleD3D12()->getCommandQueue()->flush();
            }
            return;
        }
    }

    surface->resize(w, h);
    app->getModuleD3D12()->getCommandQueue()->flush();
    m_viewports.push_back({ surface, type, width, height });
}

D3D12_GPU_VIRTUAL_ADDRESS ModuleRender::allocateInRingBuffer(const void* data, size_t size)
{
    return m_ringBuffer->allocate(data, size, app->getModuleD3D12()->getCurrentFrame());
}

void ModuleRender::renderToSurface(ID3D12GraphicsCommandList4* commandList, RenderSurface& surface, std::function<void(RenderSurface&, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE)> renderFunc)
{
    auto colorTex = surface.getTexture(RenderSurface::COLOR_0);
    auto depthTex = surface.getTexture(RenderSurface::DEPTH_STENCIL);

    transitionResource(commandList,colorTex->getD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderFunc(surface, colorTex->getRTV(0).cpu, depthTex->getDSV().cpu);

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

void ModuleRender::transitionResource( ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES  beforeState,D3D12_RESOURCE_STATES  afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}

void ModuleRender::renderScene(ID3D12GraphicsCommandList4* commandList, const RenderSurface* surface, const RenderCamera& camera, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, bool renderDebug, RenderViewType viewType)
{
    PERF_RENDER(renderDebug ? "ModuleRender::renderScene(Editor)" : "ModuleRender::renderScene(Game)");

    Texture* ssaoDepthTexture = surface ? surface->getTexture(RenderSurface::SSAO_DEPTH).get() : nullptr;
    Texture* ssaoNormalTexture = surface ? surface->getTexture(RenderSurface::SSAO_NORMAL).get() : nullptr;
    Texture* ssaoRawTexture = surface ? surface->getTexture(RenderSurface::SSAO_RAW).get() : nullptr;
    Texture* ssaoBlurTexture = surface ? surface->getTexture(RenderSurface::SSAO_BLUR).get() : nullptr;

    RenderContext ctx{
        .view = camera.view,
        .projection = camera.projection,
        .cameraPosition = camera.position,
        .viewport = viewport,
        .scissorRect = scissorRect,
        .ringBuffer = m_ringBuffer,
        .renderDebug = renderDebug,
        .viewType = viewType,
        .uiTextCommands = &app->getModuleUI()->getTextCommands(),
        .uiImageCommands = &app->getModuleUI()->getImageCommands(),
        .particleCommands = &app->getModuleParticleSystem()->getParticleCommands(),
        .skyBoxSettings = &app->getModuleScene()->getScene()->getSkyBoxSettings(),
        .shadowData = nullptr,
        .ssaoDepthTexture = ssaoDepthTexture,
        .ssaoNormalTexture = ssaoNormalTexture,
        .ssaoRawTexture = ssaoRawTexture,
        .ssaoBlurTexture = ssaoBlurTexture,
        .ssaoData = nullptr,
    };

    {
        PERF_RENDER("ModuleRender::renderScene::SkinningComputePass");

        if (m_skinningComputePass)
        {
            m_skinningComputePass->prepare(ctx);
            m_skinningComputePass->apply(commandList);
        }
    }

    {
        PERF_RENDER("ModuleRender::renderScene::ShadowMapPass");

        if (m_shadowMapPass)
        {
            if (!m_shadowMapRenderedThisFrame)
            {
                m_shadowMapPass->prepare(ctx);
                m_shadowMapPass->apply(commandList);

                m_currentShadowData = &m_shadowMapPass->getFrameData();
                m_shadowMapRenderedThisFrame = true;
            }

            ctx.shadowData = m_currentShadowData;
        }
    }

    {
        PERF_RENDER("ModuleRender::renderScene::SSAOGeometryPass");

        if (m_ssaoGeometryPass)
        {
            m_ssaoGeometryPass->prepare(ctx);
            m_ssaoGeometryPass->apply(commandList);
        }
    }

    {
        PERF_RENDER("ModuleRender::renderScene::SSAOPass");

        if (m_ssaoPass)
        {
            m_ssaoPass->prepare(ctx);
            m_ssaoPass->apply(commandList);
        }
    }


    {
        PERF_RENDER("ModuleRender::renderScene::Background");
        renderBackground(commandList, rtvHandle, dsvHandle, viewport, scissorRect);
    }


    {
        PERF_RENDER("ModuleRender::renderScene::PreparePasses");
        for (auto& pass : m_renderPasses)
        {
            pass->prepare(ctx);
        }
    }

    {
        PERF_RENDER("ModuleRender::renderScene::ApplyPasses");
        for (auto& pass : m_renderPasses)
        {
            pass->apply(commandList);
        }
    }
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

#pragma region Wrappers
void ModuleRender::renderEditorScene(ID3D12GraphicsCommandList4* commandList, const RenderSurface* surface, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    D3D12_VIEWPORT viewport = { 0, 0, width, height, 0, 1 };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

    renderScene(commandList, surface, getEditorCamera(), rtvHandle, dsvHandle, viewport, scissorRect, /*debug=*/true, RenderViewType::Editor);
}

void ModuleRender::renderPlayScene(ID3D12GraphicsCommandList4* commandList, const RenderSurface* surface, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height)
{
    const RenderCamera camera = getGameCamera();
    if (!camera.valid) return;

    D3D12_VIEWPORT viewport = { 0, 0, width, height, 0, 1 };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

    renderScene(commandList, surface, camera, rtvHandle, dsvHandle, viewport, scissorRect, m_moduleGameView->getShowDebugWindow(), RenderViewType::Game);
}

void ModuleRender::renderGameToBackbuffer(ID3D12GraphicsCommandList4* commandList, const RenderSurface* surface, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
    const RenderCamera camera = getGameCamera();
    if (!camera.valid) return;

    renderScene(commandList, surface, camera, rtvHandle, dsvHandle, viewport, scissorRect, m_moduleGameView->getShowDebugWindow(), RenderViewType::Game);
}
#pragma endregion

void ModuleRender::markDebugDrawCacheDirty()
{
    if (m_debugDrawPass)
    {
        m_debugDrawPass->markCacheDirty();
    }
}

int ModuleRender::getTrianglesCount() const { return m_meshRenderPass->getTriangleCount(); }
int ModuleRender::getMeshCount() const { return m_meshRenderPass->getMeshCount(); }
