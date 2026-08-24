#include "Globals.h"
#include "OcclusionRevealPass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleRender.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "ModuleD3D12.h"

#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"
#include "RingBuffer.h"

#include "OcclusionTargetComponent.h"

#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "LightComponent.h"

#include "BasicMaterial.h"
#include "BasicMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include "SkyBoxPass.h"
#include "SkyBox.h"
#include "BoundingBox.h"

#include <DirectXMath.h>
#include <cfloat>
#include "PlatformHelpers.h"

#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>

OcclusionRevealPass::OcclusionRevealPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    createRootSignature();
    createPipelineState();

    createBubbleRootSignature();
    createBubblePipelineState();
}

void OcclusionRevealPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParams[16] = {};

    CD3DX12_DESCRIPTOR_RANGE materialRange;
    CD3DX12_DESCRIPTOR_RANGE irradianceRange;
    CD3DX12_DESCRIPTOR_RANGE environmentRange;
    CD3DX12_DESCRIPTOR_RANGE brdfRange;
    CD3DX12_DESCRIPTOR_RANGE shadowRange;
    CD3DX12_DESCRIPTOR_RANGE mainDepthRange;
    CD3DX12_DESCRIPTOR_RANGE eligibilityRange;
    CD3DX12_DESCRIPTOR_RANGE dissolveRange;
    CD3DX12_DESCRIPTOR_RANGE samplerRange;

    materialRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
    irradianceRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
    environmentRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);
    brdfRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0);
    shadowRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11, 0);
    mainDepthRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12, 0);
    eligibilityRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 13, 0);
    dissolveRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 14, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);               // b0 MVP
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);                  // b1 Scene
    rootParams[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL);                // b2 Lights
    rootParams[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_PIXEL);                // b3 Shadows
    rootParams[4].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_ALL);                  // b4 Model
    rootParams[5].InitAsConstantBufferView(5, 0, D3D12_SHADER_VISIBILITY_PIXEL);                // b5 VFX
    rootParams[6].InitAsConstants(2, 6, 0, D3D12_SHADER_VISIBILITY_PIXEL);                      // b6 Reveal settings

    rootParams[7].InitAsDescriptorTable(1, &materialRange, D3D12_SHADER_VISIBILITY_PIXEL);      // t0..t7
    rootParams[8].InitAsDescriptorTable(1, &irradianceRange, D3D12_SHADER_VISIBILITY_PIXEL);    // t8
    rootParams[9].InitAsDescriptorTable(1, &environmentRange, D3D12_SHADER_VISIBILITY_PIXEL);   // t9
    rootParams[10].InitAsDescriptorTable(1, &brdfRange, D3D12_SHADER_VISIBILITY_PIXEL);         // t10
    rootParams[11].InitAsDescriptorTable(1, &shadowRange, D3D12_SHADER_VISIBILITY_PIXEL);       // t11
    rootParams[12].InitAsDescriptorTable(1, &mainDepthRange, D3D12_SHADER_VISIBILITY_PIXEL);    // t12
    rootParams[13].InitAsDescriptorTable(1, &eligibilityRange, D3D12_SHADER_VISIBILITY_PIXEL);  // t13
    rootParams[14].InitAsDescriptorTable(1, &dissolveRange, D3D12_SHADER_VISIBILITY_PIXEL);     // t14
    rootParams[15].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);      // s0...

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

    ThrowIfFailed(D3DReadFileToBlob(L"OcclusionRevealVS.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"OcclusionRevealPS.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
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

void OcclusionRevealPass::createBubbleRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParams[3] = {};

    CD3DX12_DESCRIPTOR_RANGE mainDepthRange;
    CD3DX12_DESCRIPTOR_RANGE eligibilityRange;

    mainDepthRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    eligibilityRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[1].InitAsDescriptorTable(1, &mainDepthRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[2].InitAsDescriptorTable(1, &eligibilityRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_bubbleRootSignature)));
}

void OcclusionRevealPass::createBubblePipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"OcclusionBubbleVS.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"OcclusionBubblePS.cso", &pixelShaderBlob));

    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    blendDesc.RenderTarget[0].BlendEnable = TRUE;

    // New RGB = Old RGB * shader output alpha.
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

    // Preserve SCENE_HDR alpha.
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.pRootSignature = m_bubbleRootSignature.Get();
    psoDesc.InputLayout = { nullptr, 0 };

    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    psoDesc.BlendState = blendDesc;

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_bubblePipelineState)));
}

void OcclusionRevealPass::prepare(const RenderContext& ctx)
{
    m_view = &ctx.view;
    m_projection = &ctx.projection;

    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    m_renderSurface = &ctx.renderSurface;

    m_meshRenderers.clear();
    m_bubbleCB = {};

    UINT bubbleIndex = 0;

    const auto& targets = app->getModuleScene()->getOcclusionTargetComponents();

    for (OcclusionTargetComponent* target : targets)
    {
        if (target == nullptr || !target->isActive())
            continue;

        GameObject* owner = target->getOwner();

        if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
            continue;

        collectMeshRenderers(owner);

        if (bubbleIndex < MAX_OCCLUSION_BUBBLES && buildBubbleForTarget(owner, bubbleIndex))
            ++bubbleIndex;
    }

    m_bubbleCB.settings = DirectX::SimpleMath::Vector4(m_depthBias, 0.0f, 0.0f, 0.0f);
    m_bubbleCBAddress = ctx.ringBuffer->allocate(&m_bubbleCB, sizeof(OcclusionBubbleCB), app->getModuleD3D12()->getCurrentFrame());

    SceneDataCB sceneData{};
    sceneData.viewPos = ctx.cameraPosition;

    const float width = std::max(1.0f, ctx.viewport.Width);
    const float height = std::max(1.0f, ctx.viewport.Height);

    sceneData.screenSize = DirectX::SimpleMath::Vector2(width, height);
    sceneData.invScreenSize = DirectX::SimpleMath::Vector2(1.0f / width, 1.0f / height);
    sceneData.renderFlags = DirectX::SimpleMath::Vector4::Zero;

    m_sceneDataCBAddress = ctx.ringBuffer->allocate(&sceneData, sizeof(SceneDataCB), app->getModuleD3D12()->getCurrentFrame());

    GPULightsConstantBuffer lightsCB = packLightsForGPU(
        app->getModuleScene()->getLightComponents(),
        LightDefaults::DEFAULT_AMBIENT_COLOR,
        LightDefaults::DEFAULT_AMBIENT_INTENSITY
    );

    m_lightsAddress = ctx.ringBuffer->allocate(&lightsCB, sizeof(GPULightsConstantBuffer), app->getModuleD3D12()->getCurrentFrame());

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

    // First weaken the eligible occluder around the projected target region.
    renderBubble(commandList, mainDepth.get(), eligibility.get(), rtv);

    // Then draw the actual PBR target exactly as in Commit 5.
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = targetDepth->getDSV().cpu;

    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
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

    commandList->SetGraphicsRootConstantBufferView(1, m_sceneDataCBAddress);
    commandList->SetGraphicsRootConstantBufferView(2, m_lightsAddress);

    float revealSettings[2] = { m_depthBias, m_revealAlpha };
    commandList->SetGraphicsRoot32BitConstants(6, 2, revealSettings, 0);

    commandList->SetGraphicsRootDescriptorTable(8, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getIrradiance()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(9, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getEnvironment()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(10, app->getModuleResources()->getEnvironmentBrdfTexture()->getSRV().gpu);

    if (m_hasShadowData && m_shadowCBAddress != 0 && m_cascadeShadowMapSRV.ptr != 0)
    {
        commandList->SetGraphicsRootConstantBufferView(3, m_shadowCBAddress);
        commandList->SetGraphicsRootDescriptorTable(11, m_cascadeShadowMapSRV);
    }

    commandList->SetGraphicsRootDescriptorTable(12, mainDepth->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(13, eligibility->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(15, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));

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

    const auto& submeshes = mesh->getSubmeshes();
    const auto& materials = renderer->getMaterials();

    if (materials.size() != submeshes.size())
        return;


    OcclusionRevealVisualEffectsCB visualEffectsCB{};
    Texture* dissolveTexture = nullptr;

    Component* damageHighlightComponent = owner->GetComponent(ComponentType::DAMAGE_HIGHLIGHT);

    if (damageHighlightComponent != nullptr)
    {
        visualEffectsCB.damageHighlightDataCB.hasDamageHighlightComponent = 1;
        visualEffectsCB.damageHighlightDataCB.damageHighlightData = static_cast<DamageHighlightComponent*>(damageHighlightComponent)->getDamageHighlightData();
    }

    Component* dissolveComponent = owner->GetComponent(ComponentType::DISSOLVE);

    if (dissolveComponent != nullptr)
    {
        DissolveComponent* dissolve = static_cast<DissolveComponent*>(dissolveComponent);
        Texture* noiseTexture = dissolve->getTexture();

        if (noiseTexture != nullptr && noiseTexture->getSRV().IsValid())
        {
            visualEffectsCB.dissolveDataCB.hasDissolveComponent = 1;
            visualEffectsCB.dissolveDataCB.dissolveData = dissolve->getDissolveData();
            dissolveTexture = noiseTexture;
        }
    }

    commandList->SetGraphicsRootConstantBufferView(5, app->getModuleRender()->allocateInRingBuffer(&visualEffectsCB, sizeof(OcclusionRevealVisualEffectsCB)));


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


    for (size_t i = 0; i < submeshes.size(); ++i)
    {
        BasicMaterial* material = materials[i].get();

        if (material == nullptr)
            continue;

        ModelData modelData{};
        modelData.model = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getGlobalMatrix().Transpose();
        modelData.normalMat = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getNormalMatrix().Transpose();
        modelData.material = material->getMaterial();

        commandList->SetGraphicsRootConstantBufferView(4, app->getModuleRender()->allocateInRingBuffer(&modelData, sizeof(ModelData)));

        commandList->SetGraphicsRootDescriptorTable(7, material->getTableGPUHandle());

        if (dissolveTexture != nullptr)
            commandList->SetGraphicsRootDescriptorTable(14, dissolveTexture->getSRV().gpu);
        else
            commandList->SetGraphicsRootDescriptorTable(14, material->getTableGPUHandle());

        commandList->DrawIndexedInstanced(submeshes[i].indexCount, 1, submeshes[i].indexStart, 0, 0);
    }
}

bool OcclusionRevealPass::buildBubbleForTarget(GameObject* targetRoot, UINT bubbleIndex)
{
    if (targetRoot == nullptr || bubbleIndex >= MAX_OCCLUSION_BUBBLES)
        return false;

    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;

    float minDepth = FLT_MAX;
    float maxDepth = -FLT_MAX;

    bool hasProjectedPoint = false;

    accumulateProjectedBounds(targetRoot, minX, minY, maxX, maxY, minDepth, maxDepth, hasProjectedPoint);

    if (!hasProjectedPoint)
        return false;

    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;

    float radiusX = std::max((maxX - minX) * 0.5f * m_bubbleScale, 1.0f);
    float radiusY = std::max((maxY - minY) * 0.5f * m_bubbleScale, 1.0f);

    // Representative depth for the complete target.
    // This is intentionally an artistic single-depth approximation for the bubble,
    // not a replacement for TARGET_DEPTH used by the actual player reveal.
    float targetDepth = std::clamp((minDepth + maxDepth) * 0.5f, 0.0f, 1.0f);

    m_bubbleCB.centerRadius[bubbleIndex] = DirectX::SimpleMath::Vector4(centerX, centerY, radiusX, radiusY);
    m_bubbleCB.depthParams[bubbleIndex] = DirectX::SimpleMath::Vector4(targetDepth, m_bubbleSoftness, m_occluderOpacity, 1.0f);

    return true;
}

void OcclusionRevealPass::accumulateProjectedBounds(
    GameObject* gameObject,
    float& minX,
    float& minY,
    float& maxX,
    float& maxY,
    float& minDepth,
    float& maxDepth,
    bool& hasProjectedPoint) const
{
    if (gameObject == nullptr || !gameObject->IsActiveInWindowHierarchy())
        return;

    MeshRenderer* renderer = gameObject->GetComponentAs<MeshRenderer>(ComponentType::MODEL);

    if (renderer != nullptr && renderer->isActive() && renderer->hasMesh())
    {
        const Vector3* points = renderer->getBoundingBox().getPoints();

        Matrix viewProjection = *m_view * *m_projection;
        DirectX::XMMATRIX viewProjectionXM = DirectX::XMLoadFloat4x4(&viewProjection);

        for (UINT i = 0; i < 8; ++i)
        {
            const Vector3& point = points[i];

            DirectX::XMVECTOR worldPoint = DirectX::XMVectorSet(point.x, point.y, point.z, 1.0f);
            DirectX::XMVECTOR clipPoint = DirectX::XMVector4Transform(worldPoint, viewProjectionXM);

            DirectX::XMFLOAT4 clip;
            DirectX::XMStoreFloat4(&clip, clipPoint);

            if (clip.w <= 0.0001f)
                continue;

            float ndcX = clip.x / clip.w;
            float ndcY = clip.y / clip.w;
            float ndcZ = clip.z / clip.w;

            if (ndcZ < 0.0f || ndcZ > 1.0f)
                continue;

            float screenX = m_viewport.TopLeftX + (ndcX * 0.5f + 0.5f) * m_viewport.Width;
            float screenY = m_viewport.TopLeftY + (-ndcY * 0.5f + 0.5f) * m_viewport.Height;

            minX = std::min(minX, screenX);
            minY = std::min(minY, screenY);

            maxX = std::max(maxX, screenX);
            maxY = std::max(maxY, screenY);

            minDepth = std::min(minDepth, ndcZ);
            maxDepth = std::max(maxDepth, ndcZ);

            hasProjectedPoint = true;
        }
    }

    Transform* transform = gameObject->GetTransform();

    if (transform == nullptr)
        return;

    for (GameObject* child : transform->getAllChildren())
        accumulateProjectedBounds(child, minX, minY, maxX, maxY, minDepth, maxDepth, hasProjectedPoint);
}

void OcclusionRevealPass::renderBubble(
    ID3D12GraphicsCommandList4* commandList,
    Texture* mainDepth,
    Texture* eligibility,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv)
{
    if (mainDepth == nullptr || eligibility == nullptr || m_bubbleCBAddress == 0)
        return;

    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_bubblePipelineState.Get());
    commandList->SetGraphicsRootSignature(m_bubbleRootSignature.Get());

    ID3D12DescriptorHeap* heaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootConstantBufferView(0, m_bubbleCBAddress);
    commandList->SetGraphicsRootDescriptorTable(1, mainDepth->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(2, eligibility->getSRV().gpu);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);
}

GPULightsConstantBuffer OcclusionRevealPass::packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const
{
    GPULightsConstantBuffer cb{};

    cb.ambientColor = ambientColor;
    cb.ambientIntensity = ambientIntensity;

    for (const LightComponent* light : lights)
    {
        if (light == nullptr || !light->isActive())
            continue;

        const GameObject* owner = light->getOwner();

        if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
            continue;

        const Transform* transform = owner->GetTransform();

        if (transform == nullptr)
            continue;

        const LightData& data = light->getData();
        const LightCommon& common = data.common;

        const Matrix& world = transform->getGlobalMatrix();
        const Vector3 pos(world._41, world._42, world._43);
        const Vector3 fwd = transform->getForward();

        switch (data.type)
        {
        case LightType::DIRECTIONAL:
            if (cb.directionalCount < LightDefaults::MAX_DIRECTIONAL_LIGHTS)
            {
                auto& gpuLight = cb.directionalLights[cb.directionalCount++];
                gpuLight.direction = fwd;
                gpuLight.color = common.color;
                gpuLight.intensity = common.intensity;
            }
            break;

        case LightType::POINT:
            if (cb.pointCount < LightDefaults::MAX_POINT_LIGHTS)
            {
                auto& gpuLight = cb.pointLights[cb.pointCount++];
                gpuLight.position = pos;
                gpuLight.radius = data.parameters.point.radius;
                gpuLight.color = common.color;
                gpuLight.intensity = common.intensity;
            }
            break;

        case LightType::SPOT:
            if (cb.spotCount < LightDefaults::MAX_SPOT_LIGHTS)
            {
                const auto& spot = data.parameters.spot;

                auto& gpuLight = cb.spotLights[cb.spotCount++];
                gpuLight.position = pos;
                gpuLight.direction = fwd;
                gpuLight.radius = spot.radius;
                gpuLight.color = common.color;
                gpuLight.intensity = common.intensity;
                gpuLight.cosineInnerAngle = std::cos(XMConvertToRadians(spot.innerAngleDegrees));
                gpuLight.cosineOuterAngle = std::cos(XMConvertToRadians(spot.outerAngleDegrees));
            }
            break;

        default:
            break;
        }
    }

    return cb;
}