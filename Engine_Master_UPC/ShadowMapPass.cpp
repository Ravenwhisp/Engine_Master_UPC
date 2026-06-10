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

#include <d3dx12.h>
#include <d3dcompiler.h>
#include "PlatformHelpers.h"

#include <cmath>

ShadowMapPass::ShadowMapPass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    m_shadowMap.reset(app->getModuleResources()->createShadowMap(SHADOW_MAP_SIZE));

    m_viewport = {};
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(SHADOW_MAP_SIZE);
    m_viewport.Height = static_cast<float>(SHADOW_MAP_SIZE);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect = {};
    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(SHADOW_MAP_SIZE);
    m_scissorRect.bottom = static_cast<LONG>(SHADOW_MAP_SIZE);

    if (m_shadowMap != nullptr)
    {
        m_shadowMapState = m_shadowMap->getDesc().initialState;
    }

    createRootSignature();
    createPipelineState();
}

void ShadowMapPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParameters[1] = {};

    rootParameters[0].InitAsConstants(
        sizeof(ShadowDrawConstants) / sizeof(UINT32),
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

const LightComponent* ShadowMapPass::findMainDirectionalLight() const
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

    if (ctx.ringBuffer != nullptr)
    {
        m_frameData.shadowCBAddress = ctx.ringBuffer->allocate(
            &shadowCB,
            sizeof(ShadowDataCB),
            app->getModuleD3D12()->getCurrentFrame());
    }
}

void ShadowMapPass::prepareDirectionalShadowData(const RenderContext& ctx, const LightComponent& light)
{
    m_frameData = {};
    m_frameData.enabled = true;

    if (m_shadowMap != nullptr && m_shadowMap->hasSRV())
    {
        m_frameData.shadowMapSRV = m_shadowMap->getSRV().gpu;
    }

    const GameObject* lightOwner = light.getOwner();
    const Transform* lightTransform = lightOwner != nullptr ? lightOwner->GetTransform() : nullptr;

    if (lightTransform == nullptr)
    {
        prepareDisabledShadowData(ctx);
        return;
    }

    Vector3 lightDirection = lightTransform->getForward();
    lightDirection.Normalize();

    const Vector3 target = ctx.cameraPosition;
    const Vector3 eye = target - lightDirection * SHADOW_LIGHT_DISTANCE;

    Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

    if (std::abs(lightDirection.y) > 0.95f)
    {
        up = Vector3(0.0f, 0.0f, 1.0f);
    }

    m_frameData.lightView = Matrix::CreateLookAt(
        eye,
        target,
        up);

    m_frameData.lightProjection = Matrix::CreateOrthographic(
        SHADOW_ORTHO_SIZE,
        SHADOW_ORTHO_SIZE,
        SHADOW_NEAR_PLANE,
        SHADOW_FAR_PLANE);

    m_frameData.lightViewProjection =
        m_frameData.lightView * m_frameData.lightProjection;

    ShadowDataCB shadowCB{};
    shadowCB.lightViewProjection = m_frameData.lightViewProjection.Transpose();
    shadowCB.shadowBias = SHADOW_BIAS;
    shadowCB.shadowStrength = SHADOW_STRENGTH;
    shadowCB.shadowsEnabled = 1;

    if (ctx.ringBuffer != nullptr)
    {
        m_frameData.shadowCBAddress = ctx.ringBuffer->allocate(
            &shadowCB,
            sizeof(ShadowDataCB),
            app->getModuleD3D12()->getCurrentFrame());
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

    Matrix global = transform->getGlobalMatrix();

    Matrix mvp = useWorldSpaceSkinnedVB
        ? m_frameData.lightViewProjection
        : global * m_frameData.lightViewProjection;

    ShadowDrawConstants constants{};
    constants.mvp = mvp.Transpose();

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

    OutputDebugStringA(
        ("[ShadowMapPass] transitionShadowMap: " +
            std::to_string(m_shadowMapState) +
            " -> " +
            std::to_string(newState) +
            "\n").c_str());

    ComPtr<ID3D12Resource> shadowResource = m_shadowMap->getD3D12Resource();

    if (shadowResource == nullptr)
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            shadowResource.Get(),
            m_shadowMapState,
            newState);

    commandList->ResourceBarrier(1, &barrier);

    m_shadowMapState = newState;
}

void ShadowMapPass::prepare(const RenderContext& ctx)
{
    m_meshRenderers = app->getModuleScene()->getVisibleMeshRenderers();

    const LightComponent* mainDirectionalLight = findMainDirectionalLight();

    if (mainDirectionalLight == nullptr)
    {
        prepareDisabledShadowData(ctx);
        return;
    }

    prepareDirectionalShadowData(ctx, *mainDirectionalLight);
}

void ShadowMapPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (commandList == nullptr)
    {
        return;
    }

    if (m_shadowMap == nullptr || !m_shadowMap->hasDSV())
    {
        return;
    }

    if (!m_frameData.enabled)
    {
        transitionShadowMap(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    renderCasters(commandList);

    transitionShadowMap(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}