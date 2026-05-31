#include "Globals.h"
#include "SkinningComputePass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "MeshRenderer.h"
#include "Skin.h"
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
    rootParameters[4].InitAsConstants(2, 0);            // b0: vertex count + palette count

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

        Skin* skin = renderer->getSkin();
        if (!skin)
            continue;

        if (skin->isCpuSkinningFallbackEnabled())
            continue;

        if (!renderer->isActive())
            continue;

        if (!renderer->hasMesh())
            continue;

        if (!skin->hasSkinPalette())
            continue;

        if (!skin->hasGpuSkinningResources())
            continue;

        const uint32_t vertexCount = skin->getSkinningVertexCount();


        if (vertexCount == 0)
            continue;
        
        const uint32_t paletteCount = static_cast<uint32_t>(skin->getMatrixPalette().size());

        auto& mesh = renderer->getMesh();
        if (!mesh || !mesh->getVertexBuffer())
            continue;

        ID3D12Resource* inputResource = mesh->getVertexBuffer()->getD3D12Resource().Get();
        ID3D12Resource* outputResource = skin->getCurrentGpuSkinnedOutputResource();
        ID3D12Resource* paletteModelResource = skin->getCurrentGpuPaletteModelResource();
        ID3D12Resource* paletteNormalResource = skin->getCurrentGpuPaletteNormalResource();

        if (!inputResource || !outputResource || !paletteModelResource || !paletteNormalResource)
            continue;

        const uint32_t threadGroupCount = (vertexCount + 63u) / 64u;

        if (paletteCount == 0)
        {
            DEBUG_ERROR("[SkinningComputePass] paletteCount is 0. owner=%s vertexCount=%u", owner->GetName().c_str(), vertexCount);
            continue;
        }

        if (threadGroupCount == 0)
        {
            DEBUG_ERROR("[SkinningComputePass] threadGroupCount is 0. owner=%s vertexCount=%u", owner->GetName().c_str(), vertexCount);
            continue;
        }

        const UINT64 expectedVertexBytes = static_cast<UINT64>(vertexCount) * sizeof(Vertex);
        const UINT64 expectedPaletteBytes = static_cast<UINT64>(paletteCount) * sizeof(Matrix);

        const D3D12_RESOURCE_DESC inputDesc = inputResource->GetDesc();
        const D3D12_RESOURCE_DESC outputDesc = outputResource->GetDesc();
        const D3D12_RESOURCE_DESC paletteModelDesc = paletteModelResource->GetDesc();
        const D3D12_RESOURCE_DESC paletteNormalDesc = paletteNormalResource->GetDesc();

        if (inputDesc.Width < expectedVertexBytes)
        {
            DEBUG_ERROR("[SkinningComputePass] Input VB too small. owner=%s width=%llu expected=%llu vertexCount=%u", owner->GetName().c_str(), inputDesc.Width, expectedVertexBytes, vertexCount);
            continue;
        }

        if (outputDesc.Width < expectedVertexBytes)
        {
            DEBUG_ERROR("[SkinningComputePass] Output VB too small. owner=%s width=%llu expected=%llu vertexCount=%u", owner->GetName().c_str(), outputDesc.Width, expectedVertexBytes, vertexCount);
            continue;
        }

        if (paletteModelDesc.Width < expectedPaletteBytes || paletteNormalDesc.Width < expectedPaletteBytes)
        {
            DEBUG_ERROR("[SkinningComputePass] Palette buffer too small. owner=%s modelWidth=%llu normalWidth=%llu expected=%llu paletteCount=%u", owner->GetName().c_str(), paletteModelDesc.Width, paletteNormalDesc.Width, expectedPaletteBytes, paletteCount);
            continue;
        }

        CD3DX12_RESOURCE_BARRIER preBarriers[2] = {};
        preBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition( inputResource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        preBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition( outputResource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->ResourceBarrier(2, preBarriers);

        commandList->SetComputeRootShaderResourceView(0, inputResource->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, outputResource->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(2, paletteModelResource->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(3, paletteNormalResource->GetGPUVirtualAddress());
        commandList->SetComputeRoot32BitConstant(4, vertexCount, 0);
        commandList->SetComputeRoot32BitConstant(4, paletteCount, 1);

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