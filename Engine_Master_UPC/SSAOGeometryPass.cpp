#include "Globals.h"
#include "SSAOGeometryPass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "RenderContext.h"
#include "Texture.h"

#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Skin.h"
#include "BasicMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include "PlatformHelpers.h"
#include "OptickProfiler.h"

#include <d3dcompiler.h>

SSAOGeometryPass::SSAOGeometryPass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].InitAsConstants(
        sizeof(Transforms) / sizeof(UINT32),
        0,
        0,
        D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(
        _countof(rootParameters),
        rootParameters,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"SSAONormalVS.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"SSAONormalPS.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&m_pipelineState)));
}

void SSAOGeometryPass::prepare(const RenderContext& ctx)
{
    PERF_RENDER("SSAOGeometryPass::prepare");

    m_view = &ctx.view;
    m_projection = &ctx.projection;

    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    m_depthTexture = ctx.ssaoDepthTexture;
    m_normalTexture = ctx.ssaoNormalTexture;

    if (!m_depthTexture || !m_normalTexture)
    {
        m_meshRenderers.clear();
        return;
    }

    m_meshRenderers = app->getModuleScene()->getVisibleMeshRenderers();
}

void SSAOGeometryPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "SsaoGeometryPass");

    if (!m_depthTexture || !m_normalTexture)
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barriersToWrite[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_normalTexture->getD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET),

        CD3DX12_RESOURCE_BARRIER::Transition(
            m_depthTexture->getD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE)
    };

    commandList->ResourceBarrier(_countof(barriersToWrite), barriersToWrite);

    const float clearNormal[] = { 0.5f, 0.5f, 1.0f, 1.0f };

    commandList->ClearRenderTargetView(
        m_normalTexture->getRTV().cpu,
        clearNormal,
        0,
        nullptr);

    commandList->ClearDepthStencilView(
        m_depthTexture->getDSV().cpu,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr);

    const D3D12_CPU_DESCRIPTOR_HANDLE normalRTV = m_normalTexture->getRTV().cpu;
    const D3D12_CPU_DESCRIPTOR_HANDLE depthDSV = m_depthTexture->getDSV().cpu;

    commandList->OMSetRenderTargets(1, &normalRTV, FALSE, &depthDSV);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    renderMeshes(commandList);

    CD3DX12_RESOURCE_BARRIER barriersToRead[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_normalTexture->getD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

        CD3DX12_RESOURCE_BARRIER::Transition(
            m_depthTexture->getD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    };

    commandList->ResourceBarrier(_countof(barriersToRead), barriersToRead);

    END_EVENT(commandList);
}

void SSAOGeometryPass::renderMeshes(ID3D12GraphicsCommandList* commandList)
{
    PERF_RENDER("SSAOGeometryPass::renderMeshes");

    if (!m_view || !m_projection)
    {
        return;
    }

    for (MeshRenderer* renderer : m_meshRenderers)
    {
        if (!renderer)
        {
            continue;
        }

        GameObject* owner = renderer->getOwner();
        if (!owner || !owner->IsActiveInWindowHierarchy())
        {
            continue;
        }

        if (!renderer->isActive())
        {
            continue;
        }

        Transform* transform = renderer->getTransform();
        if (!transform)
        {
            continue;
        }

        const auto& mesh = renderer->getMesh();
        if (!mesh)
        {
            continue;
        }

        const auto& submeshes = mesh->getSubmeshes();
        if (submeshes.empty())
        {
            continue;
        }

        const Skin* skin = renderer->getSkin();

        const VertexBuffer* staticVB = mesh->getVertexBuffer().get();
        const VertexBuffer* gpuSkinnedVB = skin ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
        const VertexBuffer* cpuSkinnedVB = skin && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;

        const bool useGpuSkinnedVB = gpuSkinnedVB != nullptr;
        const bool useCpuSkinnedVB = !useGpuSkinnedVB && cpuSkinnedVB != nullptr;
        const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

        const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : (useCpuSkinnedVB ? cpuSkinnedVB : staticVB);
        if (!activeVB)
        {
            continue;
        }

        const DirectX::SimpleMath::Matrix global = transform->getGlobalMatrix();

        Transforms transforms{};
        transforms.mvp = useWorldSpaceSkinnedVB
            ? ((*m_view) * (*m_projection)).Transpose()
            : (global * (*m_view) * (*m_projection)).Transpose();

        transforms.normalToView = useWorldSpaceSkinnedVB
            ? (*m_view).Transpose()
            : (transform->getNormalMatrix() * (*m_view)).Transpose();

        commandList->SetGraphicsRoot32BitConstants(
            0,
            sizeof(Transforms) / sizeof(UINT32),
            &transforms,
            0);

        D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
        commandList->IASetVertexBuffers(0, 1, &vbv);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        if (mesh->hasIndexBuffer())
        {
            D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();
            commandList->IASetIndexBuffer(&ibv);

            for (const Submesh& submesh : submeshes)
            {
                commandList->DrawIndexedInstanced(
                    submesh.indexCount,
                    1,
                    submesh.indexStart,
                    0,
                    0);
            }
        }
    }
}