#include "Globals.h"
#include "SkinningComputePass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include "Transform.h"
#include "BasicMesh.h"
#include "VertexBuffer.h"

#include <d3dcompiler.h>
#include "PlatformHelpers.h"

SkinningComputePass::SkinningComputePass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[5] = {};

    rootParameters[0].InitAsShaderResourceView(0);      // t0: input vertices
    rootParameters[1].InitAsUnorderedAccessView(0);     // u0: output vertices
    rootParameters[2].InitAsShaderResourceView(1);      // t1: palette model
    rootParameters[3].InitAsShaderResourceView(2);      // t2: palette normal
    rootParameters[4].InitAsConstants(1, 0);            // b0: vertex count

    rootSignatureDesc.Init(
        _countof(rootParameters),
        rootParameters,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error));

    DXCall(m_device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));

    ComPtr<ID3DBlob> computeShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"SkinningComputeShader.cso", &computeShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void SkinningComputePass::prepare(const RenderContext& ctx)
{
    m_meshRenderers = app->getModuleScene()->getMeshRenderers();
}

void SkinningComputePass::apply(ID3D12GraphicsCommandList4* commandList)
{
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetComputeRootSignature(m_rootSignature.Get());

    for (MeshRenderer* renderer : m_meshRenderers)
    {
        if (!renderer)
            continue;

        GameObject* owner = renderer->getOwner();
        if (!owner || !owner->IsActiveInWindowHierarchy())
            continue;

        if (!renderer->isActive())
            continue;

        if (!renderer->hasMesh())
            continue;

        if (!renderer->hasSkinPalette())
            continue;

        if (!renderer->hasGpuSkinningResources())
            continue;

        const uint32_t vertexCount = renderer->getSkinningVertexCount();
        if (vertexCount == 0)
            continue;

        auto& mesh = renderer->getMesh();
        if (!mesh || !mesh->getVertexBuffer())
            continue;

        ID3D12Resource* inputResource = mesh->getVertexBuffer()->getD3D12Resource().Get();
        ID3D12Resource* outputResource = renderer->getCurrentGpuSkinnedOutputResource();
        ID3D12Resource* paletteModelResource = renderer->getCurrentGpuPaletteModelResource();
        ID3D12Resource* paletteNormalResource = renderer->getCurrentGpuPaletteNormalResource();

        if (!inputResource || !outputResource || !paletteModelResource || !paletteNormalResource)
            continue;

        CD3DX12_RESOURCE_BARRIER preBarriers[2] = {};
        preBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
            inputResource,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        preBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
            outputResource,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->ResourceBarrier(2, preBarriers);

        commandList->SetComputeRootShaderResourceView(0, inputResource->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, outputResource->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(2, paletteModelResource->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(3, paletteNormalResource->GetGPUVirtualAddress());
        commandList->SetComputeRoot32BitConstant(4, vertexCount, 0);

        const uint32_t threadGroupCount = (vertexCount + 63u) / 64u;
        commandList->Dispatch(threadGroupCount, 1, 1);

        CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(outputResource);
        commandList->ResourceBarrier(1, &uavBarrier);

        CD3DX12_RESOURCE_BARRIER postBarriers[2] = {};
        postBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
            outputResource,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        postBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
            inputResource,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        commandList->ResourceBarrier(2, postBarriers);
    }
}