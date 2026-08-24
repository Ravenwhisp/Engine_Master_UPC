#include "Globals.h"
#include "OcclusionRevealPass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleRender.h"
#include "ModuleDescriptors.h"

#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"

#include "OcclusionTargetComponent.h"

#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"

#include "BasicMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include "PlatformHelpers.h"

#include <d3dcompiler.h>
#include <algorithm>


OcclusionRevealPass::OcclusionRevealPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    createRootSignature();
    createPipelineState();
}


void OcclusionRevealPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParams[4] = {};

    CD3DX12_DESCRIPTOR_RANGE mainDepthRange;
    CD3DX12_DESCRIPTOR_RANGE eligibilityRange;

    mainDepthRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    eligibilityRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);              // b0 - MVP
    rootParams[1].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);                    // b1 - Depth bias
    rootParams[2].InitAsDescriptorTable(1, &mainDepthRange, D3D12_SHADER_VISIBILITY_PIXEL);   // t0 - MainDepth
    rootParams[3].InitAsDescriptorTable(1, &eligibilityRange, D3D12_SHADER_VISIBILITY_PIXEL); // t1 - Eligibility

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}


void OcclusionRevealPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"OcclusionRevealDebugVS.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"OcclusionRevealDebugPS.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = depthStencilDesc;

    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}


void OcclusionRevealPass::prepare(const RenderContext& ctx)
{
    m_view = &ctx.view;
    m_projection = &ctx.projection;
    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;
    m_renderSurface = &ctx.renderSurface;

    m_meshRenderers.clear();

    const auto& targets = app->getModuleScene()->getOcclusionTargetComponents();

    for (OcclusionTargetComponent* target : targets)
    {
        if (target == nullptr || !target->isActive())
            continue;

        GameObject* owner = target->getOwner();

        if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
            continue;

        collectMeshRenderers(owner);
    }
}


void OcclusionRevealPass::collectMeshRenderers(GameObject* gameObject)
{
    if (gameObject == nullptr || !gameObject->IsActiveInWindowHierarchy())
        return;

    MeshRenderer* renderer = gameObject->GetComponentAs<MeshRenderer>(ComponentType::MODEL);

    if (renderer != nullptr && renderer->isActive())
    {
        if (std::find(m_meshRenderers.begin(), m_meshRenderers.end(), renderer) == m_meshRenderers.end())
            m_meshRenderers.push_back(renderer);
    }

    Transform* transform = gameObject->GetTransform();

    if (transform == nullptr)
        return;

    for (GameObject* child : transform->getAllChildren())
        collectMeshRenderers(child);
}


void OcclusionRevealPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "OcclusionRevealPass");

    std::shared_ptr<Texture> sceneHDR = m_renderSurface->getTexture(RenderSurface::SCENE_HDR);
    std::shared_ptr<Texture> mainDepth = m_renderSurface->getTexture(RenderSurface::DEPTH_STENCIL);
    std::shared_ptr<Texture> targetDepth = m_renderSurface->getTexture(RenderSurface::OCCLUSION_TARGET_DEPTH);
    std::shared_ptr<Texture> eligibility = m_renderSurface->getTexture(RenderSurface::OCCLUDER_ELIGIBILITY);

    if (!sceneHDR || !mainDepth || !targetDepth || !eligibility)
    {
        END_EVENT(commandList);
        return;
    }

    CD3DX12_RESOURCE_BARRIER mainDepthToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
        mainDepth->getD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    commandList->ResourceBarrier(1, &mainDepthToSRV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneHDR->getRTV(0).cpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = targetDepth->getDSV().cpu;

    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRoot32BitConstants(1, 1, &m_depthBias, 0);
    commandList->SetGraphicsRootDescriptorTable(2, mainDepth->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(3, eligibility->getSRV().gpu);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (MeshRenderer* renderer : m_meshRenderers)
        renderMeshRenderer(commandList, renderer);

    CD3DX12_RESOURCE_BARRIER mainDepthToDSV = CD3DX12_RESOURCE_BARRIER::Transition(
        mainDepth->getD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );

    commandList->ResourceBarrier(1, &mainDepthToDSV);

    END_EVENT(commandList);
}


void OcclusionRevealPass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer)
{
    if (renderer == nullptr || !renderer->isActive())
        return;

    GameObject* owner = renderer->getOwner();

    if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
        return;

    Transform* transform = renderer->getTransform();

    if (transform == nullptr)
        return;

    const auto& mesh = renderer->getMesh();

    if (!mesh || !mesh->hasIndexBuffer())
        return;

    const Skin* skin = renderer->getSkin();
    const VertexBuffer* gpuSkinnedVB = skin ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* cpuSkinnedVB = skin && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

    const bool useGpuSkinnedVB = gpuSkinnedVB != nullptr;
    const bool useCpuSkinnedVB = !useGpuSkinnedVB && cpuSkinnedVB != nullptr;
    const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

    const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : (useCpuSkinnedVB ? cpuSkinnedVB : staticVB);

    if (activeVB == nullptr)
        return;

    Matrix global = transform->getGlobalMatrix();
    Matrix mvp = useWorldSpaceSkinnedVB ? (*m_view * *m_projection).Transpose() : (global * *m_view * *m_projection).Transpose();

    commandList->SetGraphicsRootConstantBufferView(0, app->getModuleRender()->allocateInRingBuffer(&mvp, sizeof(Matrix)));

    D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
    D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();

    commandList->IASetVertexBuffers(0, 1, &vbv);
    commandList->IASetIndexBuffer(&ibv);

    const auto& submeshes = mesh->getSubmeshes();

    for (const auto& submesh : submeshes)
        commandList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.indexStart, 0, 0);
}