#include "Globals.h"
#include "PostProcessPass.h"

#include "RenderContext.h"
#include "RenderSurface.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "PostProcessSettings.h"
#include "Texture.h"
#include "CubeLut.h"
#include "BloomPass.h"
#include "UID.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <PlatformHelpers.h>

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

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    // Reuse the shared fullscreen-triangle vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"BRDFVertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PostProcessPixelShader.cso", &pixelShaderBlob));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.InputLayout = { nullptr, 0 };
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
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));

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
}

void PostProcessPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (!m_surface)
        return;

    auto sceneHDR = m_surface->getTexture(RenderSurface::COLOR_1);
    auto composite = m_surface->getTexture(RenderSurface::COLOR_0);
    if (!sceneHDR || !composite)
        return;

    // The scene passes left SCENE_HDR as a render target; make it readable.
    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(sceneHDR->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toSRV);

    // Bloom reads SCENE_HDR and produces its own blurred texture.
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

    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    // Leave SCENE_HDR in PIXEL_SHADER_RESOURCE for the next frame's scene pass,
    // which will transition it back to RENDER_TARGET.
}
