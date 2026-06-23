#include "Globals.h"
#include "PostProcessPass.h"

#include "RenderContext.h"
#include "RenderSurface.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleScene.h"
#include "ModuleTime.h"

#include "Scene.h"
#include "PostProcessSettings.h"
#include "PostProcessCommon.h"
#include "Texture.h"
#include "CubeLut.h"
#include "BloomPass.h"
#include "UID.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <PlatformHelpers.h>
#include <algorithm>
#include <cmath>

PostProcessPass::PostProcessPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    // Root signature: b0 (params) + t0 (HDR scene) + t1 (bloom) + t2 (LUT 3D),
    // with a static bilinear-clamp sampler at s0.
    CD3DX12_DESCRIPTOR_RANGE sceneRange, bloomRange, lutRange;
    sceneRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); 
    bloomRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
    lutRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0); 

    CD3DX12_ROOT_PARAMETER rootParameters[4] = {};
    rootParameters[0].InitAsConstants(sizeof(PostProcessParams) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &sceneRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &bloomRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, &lutRange, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC sampler = PostProcess::bilinearClampSampler();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    // Resolve into the LDR display target (COLOR_0); keep a matching depth
    // format so the depth view can stay bound for the overlay passes.
    m_pipelineState = PostProcess::createFullscreenPSO(m_device.Get(), m_rootSignature.Get(), L"PostProcessPixelShader.cso", DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

    // Bloom is produced internally and fed into the resolve.
    m_bloomPass = std::make_unique<BloomPass>(m_device);

    // Neutral identity LUT used whenever colour grading is disabled.
    m_identityLut = CubeLut::createIdentity(*m_device.Get(), 2);

    // 1x1 placeholder for the bloom slot when bloom is inactive.
    TextureDesc dummyDesc{};
    dummyDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    dummyDesc.width = 1;
    dummyDesc.height = 1;
    dummyDesc.mipLevels = 1;
    dummyDesc.views = TextureView::SRV;
    dummyDesc.initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    dummyDesc.shaderVisibleSRV = true;
    m_dummyTexture = std::make_shared<Texture>(GenerateUID(), *m_device.Get(), dummyDesc);
    m_dummyTexture->setName(L"PostProcess_DummyBloom");
}

PostProcessPass::~PostProcessPass() = default;

void PostProcessPass::prepare(const RenderContext& ctx)
{
    m_surface = &ctx.renderSurface;
    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    Scene* scene = app->getModuleScene()->getScene();
    if (!scene)
        return;

    const PostProcessSettings& settings = scene->getPostProcessSettings();

    m_params.exposure = settings.exposure;

    // Colour-grading LUT
    if (settings.lutEnabled && !settings.lutPath.empty())
    {
        if (settings.lutPath != m_loadedLutPath)
        {
            m_lutTexture = CubeLut::load(*m_device.Get(), settings.lutPath);
            m_loadedLutPath = settings.lutPath;
            if (m_lutTexture)
                m_lutSize = static_cast<int>(m_lutTexture->getDesc().depth);
        }
    }

    const bool lutActive = settings.lutEnabled && m_lutTexture != nullptr;
    m_params.enableLUT = lutActive ? 1u : 0u;
    m_params.lutSize = static_cast<float>(m_lutSize);

    m_params.enableCA = settings.chromaticAberrationEnabled ? 1u : 0u;
    m_params.caStrength = settings.chromaticAberrationStrength;

    m_runBloom = settings.bloomEnabled;
    m_params.enableBloom = settings.bloomEnabled ? 1u : 0u;
    m_params.bloomIntensity = settings.bloomIntensity;

    if (m_runBloom)
        m_bloomPass->prepare(ctx);

    // Advance time-based effects once per frame (even with several viewports).
    // Unscaled time keeps them animating in the editor / when the game is paused.
    const uint32_t frame = app->getModuleTime()->frameCount();
    const float dt = (frame != m_lastFrame) ? std::min(app->getModuleTime()->unscaledDeltaTime(), 0.05f) : 0.0f;
    m_lastFrame = frame;

    // Heartbeat / low-health damage screen effect.
    m_params.enableHeartbeat = settings.heartbeatEnabled ? 1u : 0u;
    if (settings.heartbeatEnabled)
        updateHeartbeat(settings, ctx, dt);

    // Death fade (grey then black) — independent of the heartbeat.
    updateDeathFade(settings, dt);
}

void PostProcessPass::updateDeathFade(const PostProcessSettings& settings, float dt)
{
    if (settings.deathFadeActive)
        m_deathTime += dt;
    else
        m_deathTime = 0.0f;

    const float grey = std::max(0.01f, settings.deathGreyDuration);
    const float black = std::max(0.01f, settings.deathBlackDuration);

    // First desaturate to full grey (and blur out of focus), then fade from
    // grey to black.
    m_params.deathDesat = std::min(1.0f, m_deathTime / grey);
    m_params.deathBlur = m_params.deathDesat;
    m_params.deathFade = std::min(1.0f, std::max(0.0f, (m_deathTime - grey) / black));
}

void PostProcessPass::updateHeartbeat(const PostProcessSettings& settings, const RenderContext& ctx, float dt)
{
    auto saturate01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };

    const float health = saturate01(settings.health);
    const float sep = saturate01(settings.separation);

    const float hpDanger = std::max(0.0f, 1.0f - health);
    const float sepDanger = sep;
    const bool  healthActive = health < settings.healthThreshold;
    const bool  sepActive = sepDanger > 0.05f;

    // Faster heart + sharper diastole as danger rises (ported from the preview).
    const float danger = healthActive ? hpDanger : (sepActive ? sepDanger * 0.8f : 0.0f);
    auto interBeatSeconds = [](float t) { return 0.12f + (1.0f - t) * 0.55f; };
    auto diastoleSeconds = [](float t) { return 0.18f + (1.0f - t) * 0.80f; };
    auto fireLub = [&](float t)
    {
        m_hbDubTimer = interBeatSeconds(t);
        m_hbLubTimer = -1.0f;
        m_hbPulseAnim = 1.0f;
        m_hbPulseType = 0;
    };

    // Screen sway (only when critically low health).
    m_hbSwayAngle += dt * 0.4f;
    const float critT = healthActive ? std::max(0.0f, (hpDanger - 0.5f) / 0.5f) : 0.0f;
    const float swayAmt = critT * 4.0f; // pixels
    const float swayXpx = std::sin(m_hbSwayAngle) * swayAmt;
    const float swayYpx = std::cos(m_hbSwayAngle * 0.7f) * swayAmt * 0.5f;

    // Lub-dub state machine.
    if (healthActive)
    {
        if (m_hbDubTimer < 0.0f && m_hbLubTimer < 0.0f)
            fireLub(hpDanger);

        if (m_hbDubTimer >= 0.0f)
        {
            m_hbDubTimer -= dt;
            if (m_hbDubTimer < 0.0f)
            {
                m_hbPulseAnim = 0.6f;
                m_hbPulseType = 1;
                m_hbLubTimer = diastoleSeconds(hpDanger);
            }
        }
        if (m_hbLubTimer >= 0.0f)
        {
            m_hbLubTimer -= dt;
            if (m_hbLubTimer < 0.0f)
                fireLub(hpDanger);
        }
    }
    else
    {
        m_hbDubTimer = -1.0f;
        m_hbLubTimer = -1.0f;
        m_hbPulseAnim = std::max(0.0f, m_hbPulseAnim - dt * 4.0f);
    }

    if (sepActive && m_hbDubTimer < 0.0f && m_hbLubTimer < 0.0f && !healthActive)
        fireLub(sepDanger * 0.8f);

    m_hbPulseAnim = std::max(0.0f, m_hbPulseAnim - dt * 3.5f);
    (void)danger;

    // Derived per-frame outputs fed to the shader.
    m_params.hbHealthVignette = healthActive ? std::pow(hpDanger, 1.4f) * 0.7f : 0.0f;
    m_params.hbSepVignette = sepActive ? std::pow(sepDanger, 1.3f) * 0.55f : 0.0f;
    m_params.hbPulse = m_hbPulseAnim * (m_hbPulseType == 0 ? 1.0f : 0.55f) * std::max(hpDanger, sepDanger * 0.7f);
    m_params.hbPulseIsLub = (m_hbPulseType == 0) ? 1u : 0u;
    m_params.hbCrit = critT * 0.75f;
    m_params.hbDesat = hpDanger * 0.6f + sepDanger * 0.25f;
    m_params.hbSwayX = (ctx.viewport.Width > 0.0f) ? swayXpx / ctx.viewport.Width : 0.0f;
    m_params.hbSwayY = (ctx.viewport.Height > 0.0f) ? swayYpx / ctx.viewport.Height : 0.0f;
}

void PostProcessPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (!m_surface)
        return;

    auto sceneHDR = m_surface->getTexture(RenderSurface::COLOR_1);
    auto composite = m_surface->getTexture(RenderSurface::COLOR_0);
    if (!sceneHDR || !composite)
        return;

    // The scene passes left the HDR target (COLOR_1) as a render target; make it readable.
    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(sceneHDR->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toSRV);

    // Bloom reads the HDR scene and produces its own blurred texture.
    D3D12_GPU_DESCRIPTOR_HANDLE bloomHandle = m_dummyTexture->getSRV().gpu;
    if (m_runBloom)
    {
        m_bloomPass->apply(commandList, sceneHDR->getSRV().gpu);
        bloomHandle = m_bloomPass->getBloomSRV();
    }

    const std::shared_ptr<Texture>& lut = m_lutTexture ? m_lutTexture : m_identityLut;

    auto depthTex = m_surface->getTexture(RenderSurface::DEPTH_STENCIL);
    D3D12_CPU_DESCRIPTOR_HANDLE targetRTV = composite->getRTV(0).cpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthTex->getDSV().cpu;
    commandList->OMSetRenderTargets(1, &targetRTV, FALSE, &dsv);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(PostProcessParams) / sizeof(UINT32), &m_params, 0);
    commandList->SetGraphicsRootDescriptorTable(1, sceneHDR->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(2, bloomHandle);
    commandList->SetGraphicsRootDescriptorTable(3, lut->getSRV().gpu);

    PostProcess::drawFullscreenTriangle(commandList);

    // Leave COLOR_1 (HDR) in PIXEL_SHADER_RESOURCE for the next frame's scene
    // pass, which will transition it back to RENDER_TARGET.
}
