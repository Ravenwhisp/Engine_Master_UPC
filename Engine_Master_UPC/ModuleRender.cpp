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
#include "ModuleMusic.h"

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
#include "ParticlesPass.h"
#include "ForwardPrepass.h"
#include "GeometryPass.h"
#include "DeferredShadingPass.h"
#include "PlayerPass.h"
#include "TrailPass.h"
#include "DebugDrawPass.h"
#include "UIImagePass.h"
#include "FontPass.h"
#include "StaticTexturesPass.h"
#include "SkinningComputePass.h"
#include "ShadowMapPass.h"
#include "SSAOGeometryPass.h"
#include "SSAOPass.h"
#include "SSAOBlurPass.h"
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

    m_renderPasses.push_back(std::make_unique<SkinningComputePass>(device));

    m_forwardPrepass = new ForwardPrepass(device);
    m_renderPasses.push_back(std::unique_ptr<ForwardPrepass>(m_forwardPrepass));

    m_geometryPass = new GeometryPass(device);
    m_renderPasses.push_back(std::unique_ptr<GeometryPass>(m_geometryPass));

    m_meshRenderPass = new DeferredShadingPass(device);
    m_renderPasses.push_back(std::unique_ptr<DeferredShadingPass>(m_meshRenderPass));

    m_renderPasses.push_back(std::make_unique<PlayerPass>(device));

    m_skinningComputePass = std::make_unique<SkinningComputePass>(device);
    m_shadowMapPass = std::make_unique<ShadowMapPass>(device);
    m_ssaoGeometryPass = std::make_unique<SSAOGeometryPass>(device);
    m_ssaoPass = std::make_unique<SSAOPass>(device);
    m_ssaoBlurPass = std::make_unique<SSAOBlurPass>(device);

    auto skyBoxPass = std::make_unique<SkyBoxPass>(device, app->getModuleScene()->getScene()->getSkyBoxSettings());
    m_skyBoxPass = skyBoxPass.get();

    m_renderPasses.push_back(std::move(skyBoxPass));
    m_renderPasses.push_back(std::make_unique<ParticlesPass>(device));
    m_renderPasses.push_back(std::make_unique<TrailPass>(device));
    m_renderPasses.push_back(std::move(debugDrawPass));
    m_renderPasses.push_back(std::make_unique<UIImagePass>(device));
    m_renderPasses.push_back(std::make_unique<FontPass>(device));

    // ImGui lives outside the pass list because startFrame() / apply() must
    // bracket the entire editor render, not just the scene render.
    m_imGuiPass = std::make_unique<ImGuiPass>(device, d3d12->getWindowHandle(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getCPUHandle(0), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getGPUHandle(0));



    #ifdef GAME_RELEASE
        initViewportGBuffers(d3d12->getSwapChain()->getRenderSurface(), 1920, 1080);
    #endif



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

        // Corta toda la música/SFX en curso al salir de play mode, para que no sigan
        // sonando tras el Stop del editor.
        app->getModuleMusic()->stopAllSounds();

        m_pendingStopSimulation = false;
    }

    m_ringBuffer->free(app->getModuleD3D12()->getLastCompletedFrame());

    auto* commandList = app->getModuleD3D12()->getCommandList();
    auto* swapChain = app->getModuleD3D12()->getSwapChain();

    // Resolve pending viewport resizes
    for (ViewportEntry& entry : m_viewports)
    {
        if (entry.pendingResize)
        {
            entry.width = entry.pendingResizeWidth;
            entry.height = entry.pendingResizeHeight;
            app->getModuleD3D12()->getCommandQueue()->flush();
            entry.surface->resize(entry.width, entry.height);
            entry.pendingResize = false;
        }
    }

#ifndef GAME_RELEASE
    {
        m_shadowMapRenderedThisFrame = false;
        m_currentShadowData = nullptr;

        auto* commandList = app->getModuleD3D12()->getCommandList();

        PERF_RENDER("ModuleRender::RenderViewports");
        for (const ViewportEntry& entry : m_viewports)
        {
            if (!entry.isVisible)
            {
                continue;
            }

            DEBUG_LOG(
                "Rendering viewport: type=%s surface=%p size=%f,%f visible=%d",
                entry.type == ViewportType::EDITOR ? "EDITOR" : "PLAY",
                entry.surface,
                entry.width,
                entry.height,
                entry.isVisible
            );

            BEGIN_EVENT(commandList, "Prepare Render Target");
            
            auto colorTex = entry.surface->getTexture(RenderSurface::COMPOSITE);
            transitionResource(commandList, colorTex->getD3D12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

            renderBackground(commandList, *entry.surface);

            END_EVENT(commandList);

            if (entry.type == ViewportType::EDITOR)
            {
                PERF_RENDER("ModuleRender::RenderEditorScene");
                renderEditorScene(commandList, *entry.surface);
            }
            else
            {
                PERF_RENDER("ModuleRender::RenderPlayScene");
                renderPlayScene(commandList, *entry.surface);
            }

            transitionResource(commandList,  colorTex->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET,  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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

    transitionResource(commandList,swapChain->getRenderSurface().getTexture(RenderSurface::COMPOSITE)->getD3D12Resource(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);
#ifndef GAME_RELEASE
    renderBackground(commandList, swapChain->getRenderSurface());
#else
    renderGameToBackbuffer(commandList, swapChain->getRenderSurface());
#endif
    m_imGuiPass->apply(commandList);

    transitionResource(commandList, swapChain->getRenderSurface().getTexture(RenderSurface::COMPOSITE)->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

bool ModuleRender::cleanUp()
{
    if (app != nullptr &&
        app->getModuleD3D12() != nullptr &&
        app->getModuleD3D12()->getCommandQueue() != nullptr)
    {
        app->getModuleD3D12()->getCommandQueue()->flush();
    }

    m_ssaoBlurPass.reset();
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

    initViewportGBuffers(*surface, w, h);
    surface->resize(w, h);
    app->getModuleD3D12()->getCommandQueue()->flush();
    m_viewports.push_back({ surface, type, width, height });
}

void ModuleRender::setViewportPendingResize(RenderSurface* surface, ViewportType type, float width, float height)
{
    for (ViewportEntry& entry : m_viewports)
    {
        if (entry.surface == surface)
        {
            entry.pendingResize = true;
            entry.pendingResizeWidth = width;
            entry.pendingResizeHeight = height;
        }
    }
}

void ModuleRender::setViewportVisible(RenderSurface* surface, bool isVisible)
{
    for (ViewportEntry& entry : m_viewports)
    {
        if (entry.surface == surface)
        {
            entry.isVisible = isVisible;
        }
    }
}

void ModuleRender::unregisterViewport(RenderSurface* surface)
{
    if (!surface) return;

    auto it = std::find_if(m_viewports.begin(), m_viewports.end(),[surface](const ViewportEntry& entry) { return entry.surface == surface; });

    if (it != m_viewports.end())
    {
        m_viewports.erase(it);
    }
}

void ModuleRender::initViewportGBuffers(RenderSurface& surface, float width, float height)
{
    ID3D12Device* device = app->getModuleD3D12()->getDevice();
    DescriptorHeap& srvHeap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    std::wstring bufferNames[] = { L"difusse", L"MRA", L"normal", L"position", L"emissive" };

    for (UINT i = 0; i < GeometryPass::GBUFFER_COUNT; ++i)
    {
        auto tex = std::shared_ptr<Texture>(app->getModuleResources()->createGBuffer(width, height, GeometryPass::GBUFFER_FORMATS[i]));
        //tex->setName(L"GBuffer_" + std::to_wstring(i));
        tex->setName(L"GBuffer_" + bufferNames[i]);
        surface.attachTexture(GeometryPass::kSlots[i], tex);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS ModuleRender::allocateInRingBuffer(const void* data, size_t size)
{
    return m_ringBuffer->allocate(data, size, app->getModuleD3D12()->getCurrentFrame());
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

void ModuleRender::transitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES  beforeState,D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}

void ModuleRender::renderScene(ID3D12GraphicsCommandList4* commandList, const RenderCamera& camera, RenderSurface& outputSurface, bool renderDebug, RenderViewType viewType) 
{
    PERF_RENDER(renderDebug ? "ModuleRender::renderScene(Editor)" : "ModuleRender::renderScene(Game)");

    const float w = static_cast<float>(outputSurface.getWidth());
    const float h = static_cast<float>(outputSurface.getHeight());

    DEBUG_LOG(
        "RenderSurface size: w=%u h=%u",
        outputSurface.getWidth(),
        outputSurface.getHeight()
    );

    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, w, h, 0.0f, 1.0f };
    D3D12_RECT     scissorRect = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };

    Texture* ssaoDepthTexture = outputSurface.getTexture(RenderSurface::SSAO_DEPTH).get();
    Texture* ssaoNormalTexture = outputSurface.getTexture(RenderSurface::SSAO_NORMAL).get();
    Texture* ssaoRawTexture = outputSurface.getTexture(RenderSurface::SSAO_RAW).get();
    Texture* ssaoBlurTexture = outputSurface.getTexture(RenderSurface::SSAO_BLUR).get();

    const SSAOSettings* ssaoSettings = &app->getModuleScene()->getScene()->getSSAOSettings();
    const bool ssaoEnabled = ssaoSettings ? ssaoSettings->enabled : true;
    const bool ssaoBlurEnabled = ssaoSettings ? ssaoSettings->blurEnabled : true;

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
        .renderSurface = outputSurface,
        .shadowData = nullptr,
        .ssaoDepthTexture = ssaoDepthTexture,
        .ssaoNormalTexture = ssaoNormalTexture,
        .ssaoRawTexture = ssaoRawTexture,
        .ssaoBlurTexture = ssaoBlurTexture,
        .ssaoSettings = ssaoSettings,
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

        if (m_ssaoGeometryPass && ssaoEnabled)
        {
            m_ssaoGeometryPass->prepare(ctx);
            m_ssaoGeometryPass->apply(commandList);
        }
    }

    {
        PERF_RENDER("ModuleRender::renderScene::SSAOPass");

        if (m_ssaoPass && ssaoEnabled)
        {
            m_ssaoPass->prepare(ctx);
            m_ssaoPass->apply(commandList);
        }
    }

    {
        PERF_RENDER("ModuleRender::renderScene::SSAOBlurPass");

        if (m_ssaoBlurPass && ssaoEnabled && ssaoBlurEnabled)
        {
            m_ssaoBlurPass->prepare(ctx);
            m_ssaoBlurPass->apply(commandList);
        }
    }

    m_currentSSAOData = {};

    Texture* finalSSAOTexture = nullptr;

    if (ssaoEnabled)
    {
        finalSSAOTexture = ssaoBlurEnabled && ctx.ssaoBlurTexture
            ? ctx.ssaoBlurTexture
            : ctx.ssaoRawTexture;
    }

    if (finalSSAOTexture)
    {
        m_currentSSAOData.ssaoSRV = finalSSAOTexture->getSRV().gpu;
        m_currentSSAOData.width = static_cast<uint32_t>(viewport.Width);
        m_currentSSAOData.height = static_cast<uint32_t>(viewport.Height);
        m_currentSSAOData.enabled = ssaoEnabled;

        ctx.ssaoData = &m_currentSSAOData;
    }


    {
        PERF_RENDER("ModuleRender::renderScene::Background");
        renderBackground(commandList, outputSurface);
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

void ModuleRender::renderBackground(ID3D12GraphicsCommandList4* commandList, const RenderSurface& surface)
{
    auto colorTex = surface.getTexture(RenderSurface::COMPOSITE);
    auto depthTex = surface.getTexture(RenderSurface::DEPTH_STENCIL);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = colorTex->getRTV(0).cpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthTex->getDSV().cpu;

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    const float w = static_cast<float>(surface.getWidth());
    const float h = static_cast<float>(surface.getHeight());
    D3D12_VIEWPORT vp = { 0.0f, 0.0f, w, h, 0.0f, 1.0f };
    D3D12_RECT     sr = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
    commandList->RSSetViewports(1, &vp);
    commandList->RSSetScissorRects(1, &sr);
}

#pragma region Wrappers

void ModuleRender::renderEditorScene(ID3D12GraphicsCommandList4* commandList,
    RenderSurface& outputSurface)
{
    renderScene(commandList, getEditorCamera(), outputSurface, /*renderDebug=*/true, RenderViewType::Editor);
}

void ModuleRender::renderPlayScene(ID3D12GraphicsCommandList4* commandList,
    RenderSurface& outputSurface)
{
    const RenderCamera camera = getGameCamera();
    if (!camera.valid) return;

    renderScene(commandList, camera, outputSurface, m_moduleGameView->getShowDebugWindow(), RenderViewType::Game);
}

void ModuleRender::renderGameToBackbuffer(ID3D12GraphicsCommandList4* commandList,
    RenderSurface& outputSurface)
{
    const RenderCamera camera = getGameCamera();
    if (!camera.valid) return;

    renderScene(commandList, camera, outputSurface, m_moduleGameView->getShowDebugWindow(), RenderViewType::Game);
}

#pragma endregion

void ModuleRender::markDebugDrawCacheDirty()
{
    if (m_debugDrawPass)
    {
        m_debugDrawPass->markCacheDirty();
    }
}

int ModuleRender::getTrianglesCount() const { return m_geometryPass->getTriangleCount(); }
int ModuleRender::getMeshCount() const { return m_geometryPass->getMeshCount(); }
