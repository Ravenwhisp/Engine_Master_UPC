#include "Globals.h"
#include "OcclusionTargetDepthPass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleRender.h"
#include "ModuleDescriptors.h"

#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"

#include "OcclusionTargetComponent.h"
#include "DissolveComponent.h"

#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"

#include "BasicMaterial.h"
#include "BasicMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include "PlatformHelpers.h"

#include <d3dcompiler.h>
#include <algorithm>


OcclusionTargetDepthPass::OcclusionTargetDepthPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    createRootSignature();
    createPipelineState();
}


void OcclusionTargetDepthPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParams[5] = {};

    CD3DX12_DESCRIPTOR_RANGE diffuseRange;
    CD3DX12_DESCRIPTOR_RANGE dissolveRange;
    CD3DX12_DESCRIPTOR_RANGE samplerRange;

    diffuseRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    dissolveRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);            // b0 - MVP
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);             // b1 - Coverage data
    rootParams[2].InitAsDescriptorTable(1, &diffuseRange, D3D12_SHADER_VISIBILITY_PIXEL);   // t0 - Diffuse
    rootParams[3].InitAsDescriptorTable(1, &dissolveRange, D3D12_SHADER_VISIBILITY_PIXEL);  // t8 - Dissolve noise
    rootParams[4].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);   // s0...

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}


void OcclusionTargetDepthPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"OcclusionTargetDepthVS.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"OcclusionTargetDepthPS.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.NumRenderTargets = 0;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}


void OcclusionTargetDepthPass::prepare(const RenderContext& ctx)
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


void OcclusionTargetDepthPass::collectMeshRenderers(GameObject* gameObject)
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


void OcclusionTargetDepthPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "OcclusionTargetDepthPass");

    std::shared_ptr<Texture> targetDepth = m_renderSurface->getTexture(RenderSurface::OCCLUSION_TARGET_DEPTH);

    if (!targetDepth)
    {
        END_EVENT(commandList);
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = targetDepth->getDSV().cpu;

    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
    commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    commandList->SetGraphicsRootDescriptorTable(4, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (MeshRenderer* renderer : m_meshRenderers)
        renderMeshRenderer(commandList, renderer);

    END_EVENT(commandList);
}


void OcclusionTargetDepthPass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer)
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

    if (!mesh)
        return;

    const auto& submeshes = mesh->getSubmeshes();
    const auto& materials = renderer->getMaterials();

    if (materials.size() != submeshes.size())
        return;

    const Skin* skin = renderer->getSkin();
    const VertexBuffer* gpuSkinnedVB = skin ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* cpuSkinnedVB = skin && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

    const bool useGpuSkinnedVB = gpuSkinnedVB != nullptr;
    const bool useCpuSkinnedVB = !useGpuSkinnedVB && cpuSkinnedVB != nullptr;
    const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

    const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : (useCpuSkinnedVB ? cpuSkinnedVB : staticVB);

    if (activeVB == nullptr || !mesh->hasIndexBuffer())
        return;

    Matrix global = transform->getGlobalMatrix();
    Matrix mvp = useWorldSpaceSkinnedVB ? (*m_view * *m_projection).Transpose() : (global * *m_view * *m_projection).Transpose();

    commandList->SetGraphicsRootConstantBufferView(0, app->getModuleRender()->allocateInRingBuffer(&mvp, sizeof(Matrix)));

    D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
    D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();

    commandList->IASetVertexBuffers(0, 1, &vbv);
    commandList->IASetIndexBuffer(&ibv);

    DissolveComponent* dissolve = nullptr;
    Texture* dissolveTexture = nullptr;

    Component* dissolveComponent = owner->GetComponent(ComponentType::DISSOLVE);

    if (dissolveComponent != nullptr)
    {
        DissolveComponent* candidate = static_cast<DissolveComponent*>(dissolveComponent);
        Texture* candidateTexture = candidate->getTexture();

        if (candidateTexture != nullptr && candidateTexture->getSRV().IsValid())
        {
            dissolve = candidate;
            dissolveTexture = candidateTexture;
        }
    }

    for (size_t i = 0; i < submeshes.size(); ++i)
    {
        BasicMaterial* material = materials[i].get();

        if (material == nullptr)
            continue;

        OcclusionTargetCoverageCB coverageCB{};
        coverageCB.hasDiffuseTex = material->getMaterial().hasDiffuseTex != 0 ? 1u : 0u;

        if (dissolve != nullptr)
        {
            coverageCB.hasDissolveComponent = 1u;
            coverageCB.dissolveAmount = dissolve->getDissolveAmount();
        }

        commandList->SetGraphicsRootConstantBufferView(1, app->getModuleRender()->allocateInRingBuffer(&coverageCB, sizeof(OcclusionTargetCoverageCB)));

        // BasicMaterial's descriptor table starts with SLOT_DIFFUSE (t0).
        commandList->SetGraphicsRootDescriptorTable(2, material->getTableGPUHandle());

        // Keep t8 bound to a valid descriptor even when dissolve is disabled.
        // The shader will only sample it when hasDissolveComponent != 0.
        if (dissolveTexture != nullptr)
            commandList->SetGraphicsRootDescriptorTable(3, dissolveTexture->getSRV().gpu);
        else
            commandList->SetGraphicsRootDescriptorTable(3, material->getTableGPUHandle());

        commandList->DrawIndexedInstanced(submeshes[i].indexCount, 1, submeshes[i].indexStart, 0, 0);
    }
}