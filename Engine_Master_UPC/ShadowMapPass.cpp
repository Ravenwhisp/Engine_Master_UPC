#include "Globals.h"
#include "ShadowMapPass.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleScene.h"
#include "ModuleD3D12.h"
#include "RenderContext.h"

#include "LightComponent.h"
#include "Lights.h"
#include "GameObject.h"
#include "Transform.h"
#include "RingBuffer.h"

#include "MeshRenderer.h"
#include "BasicMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Skin.h"
#include "ShadowFrustumComputePass.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include "PlatformHelpers.h"

#include <cmath>
#include <algorithm>
#include <limits>

ShadowMapPass::ShadowMapPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass)
    : m_device(device), m_shadowFrustumComputePass(shadowFrustumComputePass)
{
    createShadowMap(DEFAULT_SHADOW_MAP_SIZE);

    createRootSignature();
    createPipelineState();
}

void ShadowMapPass::createShadowMap(uint32_t size)
{
    m_currentShadowMapSize = size;

    m_shadowMap.reset(app->getModuleResources()->createShadowMap(size));

    updateShadowViewportAndScissor(size);

    if (m_shadowMap != nullptr)
    {
        m_shadowMapState = m_shadowMap->getDesc().initialState;
    }
    else
    {
        m_shadowMapState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}

void ShadowMapPass::resizeShadowMapIfNeeded(uint32_t size)
{
    if (size == 0)
    {
        size = DEFAULT_SHADOW_MAP_SIZE;
    }

    if (size == m_currentShadowMapSize && m_shadowMap != nullptr)
    {
        return;
    }

    createShadowMap(size);
}

void ShadowMapPass::updateShadowViewportAndScissor(uint32_t size)
{
    m_viewport = {};
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(size);
    m_viewport.Height = static_cast<float>(size);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect = {};
    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(size);
    m_scissorRect.bottom = static_cast<LONG>(size);
}

void ShadowMapPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParameters[2] = {};

    // b0: model matrix, changed for each mesh.
    rootParameters[0].InitAsConstants(
        sizeof(ShadowDrawConstants) / sizeof(UINT32),
        0,
        0,
        D3D12_SHADER_VISIBILITY_VERTEX);

    // b1: light view-projection matrix.
    rootParameters[1].InitAsConstantBufferView(
        1,
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
}

void ShadowMapPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"ShadowMapVertexShader.cso", &vertexShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = {};

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 0;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&m_pipelineState)));
}

const LightComponent* ShadowMapPass::findMainShadowCastingDirectionalLight() const
{
    const std::vector<LightComponent*>& lights = app->getModuleScene()->getLightComponents();

    for (const LightComponent* light : lights)
    {
        if (light == nullptr)
        {
            continue;
        }

        if (!light->isActive())
        {
            continue;
        }

        const GameObject* owner = light->getOwner();
        if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
        {
            continue;
        }

        const LightData& data = light->getData();

        if (data.type != LightType::DIRECTIONAL)
        {
            continue;
        }

        if (!data.shadow.castShadows)
        {
            continue;
        }

        return light;
    }

    return nullptr;
}

void ShadowMapPass::prepareDisabledShadowData(const RenderContext& ctx)
{
    m_frameData = {};
    m_frameData.enabled = false;

    if (m_shadowMap != nullptr && m_shadowMap->hasSRV())
    {
        m_frameData.shadowMapSRV = m_shadowMap->getSRV().gpu;
    }

    ShadowDataCB shadowCB{};
    shadowCB.lightViewProjection = Matrix::Identity.Transpose();
    shadowCB.shadowBias = SHADOW_BIAS;
    shadowCB.shadowStrength = SHADOW_STRENGTH;
    shadowCB.shadowsEnabled = 0;
    shadowCB.shadowMapTexelSize = Vector2(
        1.0f / static_cast<float>(m_currentShadowMapSize),
        1.0f / static_cast<float>(m_currentShadowMapSize));
    shadowCB.pcfEnabled = 0;
    shadowCB.pcfRadius = 1;

    if (ctx.ringBuffer != nullptr)
    {
        m_frameData.shadowCBAddress = ctx.ringBuffer->allocate(
            &shadowCB,
            sizeof(ShadowDataCB),
            app->getModuleD3D12()->getCurrentFrame());
    }
}

void ShadowMapPass::prepareDirectionalShadowData(
    const RenderContext& ctx,
    const LightComponent& light)
{
    const LightShadowSettings& shadowSettings =
        light.getData().shadow;

    resizeShadowMapIfNeeded(
        shadowSettings.shadowMapSize);

    if (m_shadowFrustumComputePass == nullptr ||
        !m_shadowFrustumComputePass->hasValidResult())
    {
        prepareDisabledShadowData(ctx);
        return;
    }

    const D3D12_GPU_VIRTUAL_ADDRESS shadowDataAddress =
        m_shadowFrustumComputePass
        ->getShadowDataBufferAddress();

    if (shadowDataAddress == 0)
    {
        prepareDisabledShadowData(ctx);
        return;
    }

    m_frameData = {};
    m_frameData.enabled = true;
    m_frameData.shadowCBAddress = shadowDataAddress;

    if (m_shadowMap != nullptr &&
        m_shadowMap->hasSRV())
    {
        m_frameData.shadowMapSRV =
            m_shadowMap->getSRV().gpu;
    }
}

void ShadowMapPass::renderCasters(ID3D12GraphicsCommandList4* commandList)
{
    for (MeshRenderer* renderer : m_meshRenderers)
    {
        if (renderer == nullptr)
        {
            continue;
        }

        renderMeshRenderer(commandList, *renderer);
    }
}

void ShadowMapPass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer& renderer)
{
    GameObject* owner = renderer.getOwner();

    if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
    {
        return;
    }

    if (!renderer.isActive())
    {
        return;
    }

    Transform* transform = renderer.getTransform();

    if (transform == nullptr)
    {
        return;
    }

    const std::shared_ptr<BasicMesh>& mesh = renderer.getMesh();

    if (mesh == nullptr)
    {
        return;
    }

    const Skin* skin = renderer.getSkin();

    const VertexBuffer* gpuSkinnedVB =
        skin != nullptr ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;

    const VertexBuffer* cpuSkinnedVB =
        skin != nullptr && skin->isCpuSkinningFallbackEnabled()
        ? skin->getCpuSkinnedVertexBuffer()
        : nullptr;

    const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

    const bool useGpuSkinnedVB = gpuSkinnedVB != nullptr;
    const bool useCpuSkinnedVB = !useGpuSkinnedVB && cpuSkinnedVB != nullptr;
    const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

    const VertexBuffer* activeVB =
        useGpuSkinnedVB ? gpuSkinnedVB :
        useCpuSkinnedVB ? cpuSkinnedVB :
        staticVB;

    if (activeVB == nullptr)
    {
        return;
    }

    const Matrix model =
        useWorldSpaceSkinnedVB
        ? Matrix::Identity
        : transform->getGlobalMatrix();

    ShadowDrawConstants constants{};
    constants.model = model.Transpose();

    commandList->SetGraphicsRoot32BitConstants(
        0,
        sizeof(ShadowDrawConstants) / sizeof(UINT32),
        &constants,
        0);

    D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    if (!mesh->hasIndexBuffer())
    {
        return;
    }

    D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();
    commandList->IASetIndexBuffer(&ibv);

    const std::vector<Submesh>& submeshes = mesh->getSubmeshes();

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

void ShadowMapPass::transitionShadowMap(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState)
{
    if (commandList == nullptr || m_shadowMap == nullptr)
    {
        return;
    }

    if (m_shadowMapState == newState)
    {
        return;
    }

    ComPtr<ID3D12Resource> shadowResource = m_shadowMap->getD3D12Resource();

    if (shadowResource == nullptr)
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(shadowResource.Get(), m_shadowMapState, newState);

    commandList->ResourceBarrier(1, &barrier);

    m_shadowMapState = newState;
}

void ShadowMapPass::prepare(const RenderContext& ctx)
{
    m_meshRenderers = app->getModuleScene()->getMeshRenderers();

    const LightComponent* mainDirectionalLight = findMainShadowCastingDirectionalLight();

    if (mainDirectionalLight == nullptr)
    {
        prepareDisabledShadowData(ctx);
        return;
    }

    prepareDirectionalShadowData(ctx, *mainDirectionalLight);
}

void ShadowMapPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "ShadowMapPass");

    if (commandList == nullptr)
    {
        END_EVENT(commandList);
        return;
    }

    if (m_shadowMap == nullptr || !m_shadowMap->hasDSV())
    {
        END_EVENT(commandList);
        return;
    }

    if (!m_frameData.enabled)
    {
        transitionShadowMap(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        END_EVENT(commandList);
        return;
    }

    const D3D12_GPU_VIRTUAL_ADDRESS shadowDataAddress = m_frameData.shadowCBAddress;

    if (shadowDataAddress == 0)
    {
        transitionShadowMap(
            commandList,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        END_EVENT(commandList);
        return;
    }

    transitionShadowMap(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE shadowDSV = m_shadowMap->getDSV().cpu;

    commandList->OMSetRenderTargets(
        0,
        nullptr,
        false,
        &shadowDSV);

    commandList->ClearDepthStencilView(
        shadowDSV,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->SetGraphicsRootConstantBufferView(1, shadowDataAddress);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    renderCasters(commandList);

    transitionShadowMap(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    END_EVENT(commandList);
}