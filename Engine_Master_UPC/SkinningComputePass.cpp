#include "Globals.h"
#include "SkinningComputePass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "MeshRenderer.h"

#include <d3dcompiler.h>
#include "PlatformHelpers.h"

SkinningComputePass::SkinningComputePass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[4] = {};

    rootParameters[0].InitAsShaderResourceView(0);      // input vertices
    rootParameters[1].InitAsUnorderedAccessView(0);     // output vertices
    rootParameters[2].InitAsShaderResourceView(1);      // palette model
    rootParameters[3].InitAsShaderResourceView(2);      // palette normal

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

    // Dispatch will be added in the next commit.
}