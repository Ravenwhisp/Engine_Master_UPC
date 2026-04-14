#include "Globals.h"
#include "SkyBoxPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleScene.h"
#include "ModuleAssets.h"

#include "SkyBoxSettings.h"
#include "SkyBox.h"
#include "SkyboxParams.h"
#include "Texture.h"
#include "TextureAsset.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <PlatformHelpers.h>
#include "MD5Fwd.h"

SkyBoxPass::SkyBoxPass(ComPtr<ID3D12Device4> device, SkyBoxSettings& settings) : m_device(device)
{
    setSettings(settings);

    ComPtr<ID3D12RootSignature> rootSignature;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParameters[0].InitAsConstants(sizeof(SkyboxParams) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    ComPtr<ID3D12PipelineState> pso;

#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"SkyBoxVertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"SkyBoxPixelShader.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    desc.pRootSignature = m_rootSignature.Get();
    desc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    desc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc.DepthStencilState.DepthEnable = TRUE;
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    desc.SampleMask = UINT_MAX;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));
}

void SkyBoxPass::prepare(const RenderContext& ctx)
{
    m_view = &ctx.view;
    m_projection = &ctx.projection;

    if (ctx.skyBoxSettings && !(*ctx.skyBoxSettings == m_lastSettings))
    {
        setSettings(*ctx.skyBoxSettings);

        m_lastSettings = *ctx.skyBoxSettings;
    }
}

void SkyBoxPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (!m_skyBox) return;
    if (!m_skyBox->getTexture()) return;
    if (!m_view || !m_projection) return;

    Matrix view = *m_view;
    view._41 = 0.0f;
    view._42 = 0.0f;
    view._43 = 0.0f;
    Matrix vp = view * *m_projection;

    vp = vp.Transpose();

    SkyboxParams params{};
    params.vp = vp;
    params.flipX = 0;
    params.flipZ = 0;

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(SkyboxParams) / sizeof(UINT32), &params, 0);
    commandList->SetGraphicsRootDescriptorTable(1, m_skyBox->getTexture()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(2, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = m_skyBox->getVertexBuffer()->getVertexBufferView();
    D3D12_INDEX_BUFFER_VIEW  indexBufferView = m_skyBox->getIndexBuffer()->getIndexBufferView();

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);

    commandList->DrawIndexedInstanced(static_cast<UINT>(m_skyBox->getIndexBuffer()->getNumIndices()), 1, 0, 0, 0);
}

void SkyBoxPass::setSettings(const SkyBoxSettings& settings)
{
    if (!settings.enabled)
    {
        m_skyBox.reset();
        return;
    }

    if (settings.cubemapAssetId == INVALID_ASSET_ID)
    {
        m_skyBox.reset();
        return;
    }

    auto assetModule = app->getModuleAssets();

    auto asset = assetModule->load<TextureAsset>(settings.cubemapAssetId);

    if (!asset)
    {
        DEBUG_ERROR("SkyBox cubemap asset not found");
        m_skyBox.reset();
        return;
    }

    m_skyBox = std::make_unique<SkyBox>(*asset);
}
