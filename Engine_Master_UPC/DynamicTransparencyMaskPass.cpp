#include "Globals.h"
#include "DynamicTransparencyMaskPass.h"

#include "Application.h"
#include "ModuleDescriptors.h"

#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"

#include "PlatformHelpers.h"

#include <d3dcompiler.h>

DynamicTransparencyMaskPass::DynamicTransparencyMaskPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    createRootSignature();
    createPipelineState();
}

void DynamicTransparencyMaskPass::createRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE targetDepthRange;
    targetDepthRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParams[1] = {};
    rootParams[0].InitAsDescriptorTable(1, &targetDepthRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void DynamicTransparencyMaskPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"DynamicTransparencyMaskPS.cso", &pixelShaderBlob));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void DynamicTransparencyMaskPass::prepare(const RenderContext& ctx)
{
    m_renderSurface = &ctx.renderSurface;
    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;
}

void DynamicTransparencyMaskPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "DynamicTransparencyMaskPass");

    std::shared_ptr<Texture> targetDepth = m_renderSurface->getTexture(RenderSurface::OCCLUSION_TARGET_DEPTH);
    std::shared_ptr<Texture> mask = m_renderSurface->getTexture(RenderSurface::DYNAMIC_TRANSPARENCY_MASK);

    if (!targetDepth || !mask)
    {
        END_EVENT(commandList);
        return;
    }

    CD3DX12_RESOURCE_BARRIER barriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(targetDepth->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(mask->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
    };

    commandList->ResourceBarrier(_countof(barriers), barriers);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = mask->getRTV(0).cpu;
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(0, targetDepth->getSRV().gpu);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    CD3DX12_RESOURCE_BARRIER restoreBarriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(targetDepth->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
        CD3DX12_RESOURCE_BARRIER::Transition(mask->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    };

    commandList->ResourceBarrier(_countof(restoreBarriers), restoreBarriers);

    END_EVENT(commandList);
}