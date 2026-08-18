#include "Globals.h"
#include "VolumetricFogApplyPass.h"

#include "VolumetricFogComputePass.h"
#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "VolumetricFogSettings.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <PlatformHelpers.h>

VolumetricFogApplyPass::VolumetricFogApplyPass(ComPtr<ID3D12Device4> device, VolumetricFogComputePass* computePass) : m_device(device), m_computePass(computePass)
{
    createRootSignature();
    createPipelineState();
}

void VolumetricFogApplyPass::createRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE mediumRange, lightingRange, integratedRange, depthRange, samplerRange;
    mediumRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    lightingRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
    integratedRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
    depthRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[6] = {};
    rootParameters[0].InitAsConstants(sizeof(ApplyConstants) / sizeof(uint32_t), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &mediumRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &lightingRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, &integratedRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[4].InitAsDescriptorTable(1, &depthRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[5].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void VolumetricFogApplyPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"VolumetricFogApplyPixelShader.cso", &pixelShaderBlob));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void VolumetricFogApplyPass::prepare(const RenderContext& ctx)
{
    m_enabled = false;
    m_renderSurface = &ctx.renderSurface;
    m_mediumVolume = nullptr;
    m_lightingVolume = nullptr;
    m_integratedVolume = nullptr;
    m_depthTexture = nullptr;
    m_sceneHDR = nullptr;
    m_constants = {};
    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    if (m_computePass == nullptr || !m_computePass->isEnabled()) return;

    Scene* scene = app->getModuleScene()->getScene();
    if (scene == nullptr) return;

    m_mediumVolume = m_computePass->getMediumVolume();
    m_lightingVolume = m_computePass->getLightingVolume();
    m_integratedVolume = m_computePass->getIntegratedVolume();
    m_depthTexture = m_renderSurface->getTexture(RenderSurface::DEPTH_STENCIL).get();
    m_sceneHDR = m_renderSurface->getTexture(RenderSurface::SCENE_HDR).get();

    if (m_mediumVolume == nullptr || m_lightingVolume == nullptr || m_integratedVolume == nullptr || m_depthTexture == nullptr || m_sceneHDR == nullptr) return;

    const VolumetricFog::GridConstants& grid = m_computePass->getGridConstants();
    const VolumetricFogSettings& settings = scene->getVolumetricFogSettings();

    m_constants.nearDistance = grid.nearDistance;
    m_constants.maxDistance = grid.maxDistance;
    m_constants.projectionA = ctx.projection._33;
    m_constants.projectionB = ctx.projection._43;
    m_constants.gridDepth = grid.gridDepth;
    m_constants.debugView = static_cast<uint32_t>(settings.debugView);
    m_constants.debugSlice = settings.debugSlice;

    m_enabled = true;
}

void VolumetricFogApplyPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (commandList == nullptr || !m_enabled) return;

    BEGIN_EVENT(commandList, "VolumetricFog::Apply");

    CD3DX12_RESOURCE_BARRIER depthToRead = CD3DX12_RESOURCE_BARRIER::Transition(m_depthTexture->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &depthToRead);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_sceneHDR->getRTV(0).cpu;
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(ApplyConstants) / sizeof(uint32_t), &m_constants, 0);
    commandList->SetGraphicsRootDescriptorTable(1, m_mediumVolume->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(2, m_lightingVolume->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(3, m_integratedVolume->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(4, m_depthTexture->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(5, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    CD3DX12_RESOURCE_BARRIER depthToWrite = CD3DX12_RESOURCE_BARRIER::Transition(m_depthTexture->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ResourceBarrier(1, &depthToWrite);

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_depthTexture->getDSV().cpu;
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    END_EVENT(commandList);
}
