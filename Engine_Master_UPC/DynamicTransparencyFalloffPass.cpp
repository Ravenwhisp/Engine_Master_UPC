#include "Globals.h"
#include "DynamicTransparencyFalloffPass.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleRender.h"
#include "ModuleScene.h"
#include "ModuleResources.h"

#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"

#include "OcclusionOccluderComponent.h"
#include "DissolveComponent.h"

#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "DeferredShadingPass.h"
#include "VolumetricFogComputePass.h"
#include "SkyBoxPass.h"
#include "SkyBox.h"
#include "ShadowTypes.h"

#include "BasicMaterial.h"
#include "BasicMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include "PlatformHelpers.h"

#include <d3dcompiler.h>
#include <algorithm>

DynamicTransparencyFalloffPass::DynamicTransparencyFalloffPass(ComPtr<ID3D12Device4> device, DeferredShadingPass* deferredShadingPass, VolumetricFogComputePass* fogComputePass)
    : m_device(device), m_deferredShadingPass(deferredShadingPass), m_fogComputePass(fogComputePass)
{
    createRootSignature();
    createPipelineState();
}

void DynamicTransparencyFalloffPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParams[15] = {};

    CD3DX12_DESCRIPTOR_RANGE materialRange, irradianceRange, environmentRange, brdfRange, shadowRange, maskRange, dissolveRange, samplerRange, integratedFogRange;

    materialRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
    irradianceRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
    environmentRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);
    brdfRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0);
    shadowRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11, 0);
    maskRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12, 0);
    dissolveRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 13, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);
    integratedFogRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 14, 0);

    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[4].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParams[5].InitAsConstantBufferView(5, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[6].InitAsDescriptorTable(1, &materialRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[7].InitAsDescriptorTable(1, &irradianceRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[8].InitAsDescriptorTable(1, &environmentRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[9].InitAsDescriptorTable(1, &brdfRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[10].InitAsDescriptorTable(1, &shadowRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[11].InitAsDescriptorTable(1, &maskRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[12].InitAsDescriptorTable(1, &dissolveRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[13].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[14].InitAsDescriptorTable(1, &integratedFogRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void DynamicTransparencyFalloffPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"DynamicTransparencyFalloffVS.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"DynamicTransparencyFalloffPS.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void DynamicTransparencyFalloffPass::prepare(const RenderContext& ctx)
{
    m_view = &ctx.view;
    m_projection = &ctx.projection;
    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;
    m_renderSurface = &ctx.renderSurface;

    m_sceneDataCBAddress = m_deferredShadingPass != nullptr ? m_deferredShadingPass->getSceneDataCBAddress() : 0;
    m_lightsCBAddress = m_deferredShadingPass != nullptr ? m_deferredShadingPass->getLightsCBAddress() : 0;

    m_hasShadowData = ctx.shadowData != nullptr;

    if (m_hasShadowData)
    {
        m_shadowCBAddress = ctx.shadowData->shadowCBAddress;
        m_cascadeShadowMapSRV = ctx.shadowData->cascadeShadowMapSRV;
    }
    else
    {
        m_shadowCBAddress = 0;
        m_cascadeShadowMapSRV = {};
    }

    m_integratedFogVolume = nullptr;
    m_fogNearDistance = 0.0f;
    m_fogMaxDistance = 0.0f;
    m_fogProjectionA = ctx.projection._33;
    m_fogProjectionB = ctx.projection._43;
    m_fogGridDepth = 0;
    m_fogEnabled = false;

    if (m_fogComputePass != nullptr && m_fogComputePass->isEnabled())
    {
        Texture* integratedVolume = m_fogComputePass->getIntegratedVolume();

        if (integratedVolume != nullptr && integratedVolume->getSRV().IsValid())
        {
            const VolumetricFog::GridConstants& grid = m_fogComputePass->getGridConstants();

            if (grid.gridDepth > 0)
            {
                m_integratedFogVolume = integratedVolume;
                m_fogNearDistance = grid.nearDistance;
                m_fogMaxDistance = grid.maxDistance;
                m_fogGridDepth = grid.gridDepth;
                m_fogEnabled = true;
            }
        }
    }

    m_meshRenderers.clear();

    const auto& occluders = app->getModuleScene()->getOcclusionOccluderComponents();

    for (OcclusionOccluderComponent* occluder : occluders)
    {
        if (occluder == nullptr || !occluder->isActive())
            continue;

        GameObject* owner = occluder->getOwner();

        if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
            continue;

        collectMeshRenderers(owner);
    }
}

void DynamicTransparencyFalloffPass::collectMeshRenderers(GameObject* gameObject)
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

void DynamicTransparencyFalloffPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "DynamicTransparencyFalloffPass");

    std::shared_ptr<Texture> sceneHDR = m_renderSurface->getTexture(RenderSurface::SCENE_HDR);
    std::shared_ptr<Texture> mainDepth = m_renderSurface->getTexture(RenderSurface::DEPTH_STENCIL);
    std::shared_ptr<Texture> mask = m_renderSurface->getTexture(RenderSurface::DYNAMIC_TRANSPARENCY_MASK);

    if (!sceneHDR || !mainDepth || !mask || m_sceneDataCBAddress == 0 || m_lightsCBAddress == 0)
    {
        END_EVENT(commandList);
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneHDR->getRTV(0).cpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = mainDepth->getDSV().cpu;

    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->SetGraphicsRootConstantBufferView(1, m_sceneDataCBAddress);
    commandList->SetGraphicsRootConstantBufferView(2, m_lightsCBAddress);

    if (m_hasShadowData && m_shadowCBAddress != 0 && m_cascadeShadowMapSRV.ptr != 0)
    {
        commandList->SetGraphicsRootConstantBufferView(3, m_shadowCBAddress);
        commandList->SetGraphicsRootDescriptorTable(10, m_cascadeShadowMapSRV);
    }

    commandList->SetGraphicsRootDescriptorTable(7, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getIrradiance()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(8, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getEnvironment()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(9, app->getModuleResources()->getEnvironmentBrdfTexture()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(11, mask->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(13, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));

    if (m_fogEnabled && m_integratedFogVolume != nullptr)
        commandList->SetGraphicsRootDescriptorTable(14, m_integratedFogVolume->getSRV().gpu);

    ID3D12DescriptorHeap* heaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (MeshRenderer* renderer : m_meshRenderers)
        renderMeshRenderer(commandList, renderer);

    END_EVENT(commandList);
}

void DynamicTransparencyFalloffPass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer)
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

    if (activeVB == nullptr)
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

        const auto& materialData = material->getMaterial();

        ModelData modelData{};
        modelData.model = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getGlobalMatrix().Transpose();
        modelData.normalMat = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getNormalMatrix().Transpose();
        modelData.material = material->getMaterial();

        DynamicTransparencyFalloffSettingsCB falloffCB{};

        falloffCB.settings = DirectX::SimpleMath::Vector4(0.0001f, dissolve != nullptr ? 1.0f : 0.0f, dissolve != nullptr ? dissolve->getDissolveAmount() : 0.0f, m_fogEnabled ? 1.0f : 0.0f);
        falloffCB.fogDepthParams = DirectX::SimpleMath::Vector4(m_fogNearDistance, m_fogMaxDistance, m_fogProjectionA, m_fogProjectionB);
        falloffCB.fogGridParams = DirectX::SimpleMath::Vector4(static_cast<float>(m_fogGridDepth), 0.0f, 0.0f, 0.0f);

        commandList->SetGraphicsRootConstantBufferView(4, app->getModuleRender()->allocateInRingBuffer(&modelData, sizeof(ModelData)));
        commandList->SetGraphicsRootConstantBufferView(5, app->getModuleRender()->allocateInRingBuffer(&falloffCB, sizeof(DynamicTransparencyFalloffSettingsCB)));
        commandList->SetGraphicsRootDescriptorTable(6, material->getTableGPUHandle());

        if (dissolveTexture != nullptr)
            commandList->SetGraphicsRootDescriptorTable(12, dissolveTexture->getSRV().gpu);
        else
            commandList->SetGraphicsRootDescriptorTable(12, material->getTableGPUHandle());

        commandList->DrawIndexedInstanced(submeshes[i].indexCount, 1, submeshes[i].indexStart, 0, 0);
    }
}