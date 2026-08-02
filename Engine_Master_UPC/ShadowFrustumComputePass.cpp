#include "Globals.h"
#include "ShadowFrustumComputePass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "ModuleScene.h"

#include "DepthReductionPass.h"
#include "RenderContext.h"
#include "Texture.h"

#include "LightComponent.h"
#include "Lights.h"
#include "GameObject.h"
#include "Transform.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include "PlatformHelpers.h"
#include <algorithm>

ShadowFrustumComputePass::ShadowFrustumComputePass(
    ComPtr<ID3D12Device4> device,
    DepthReductionPass* depthReductionPass)
    : m_device(device)
    , m_depthReductionPass(depthReductionPass)
{
    createRootSignature();
    createPipelineState();
    createOutputBuffer();
}

void ShadowFrustumComputePass::createRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE minMaxRange;
    minMaxRange.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        1,
        0,
        0);

    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};

    // t0: final 1x1 min/max depth texture
    rootParameters[0].InitAsDescriptorTable(
        1,
        &minMaxRange,
        D3D12_SHADER_VISIBILITY_ALL);

    // u0: output lightViewProjection buffer
    rootParameters[1].InitAsUnorderedAccessView(
        0,
        0);

    // b0: inverse view, projection, light direction and fitting settings
    rootParameters[2].InitAsConstants(
        sizeof(FrustumConstants) / sizeof(uint32_t),
        0,
        0,
        D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
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
}

void ShadowFrustumComputePass::createPipelineState()
{
    ComPtr<ID3DBlob> computeShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(
        L"ShadowFrustumComputeShader.cso",
        &computeShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(
        &psoDesc,
        IID_PPV_ARGS(&m_pipelineState)));
}

void ShadowFrustumComputePass::createOutputBuffer()
{
    constexpr size_t BUFFER_SIZE =
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    m_lightViewProjectionBuffer =
        app->getModuleResources()->createDefaultBuffer(
            BUFFER_SIZE,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            "ShadowLightViewProjection");

    m_outputBufferState =
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

const LightComponent*
ShadowFrustumComputePass::findMainShadowCastingDirectionalLight() const
{
    const std::vector<LightComponent*>& lights =
        app->getModuleScene()->getLightComponents();

    for (const LightComponent* light : lights)
    {
        if (light == nullptr || !light->isActive())
        {
            continue;
        }

        const GameObject* owner = light->getOwner();

        if (owner == nullptr ||
            !owner->IsActiveInWindowHierarchy())
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

void ShadowFrustumComputePass::prepare(const RenderContext& ctx)
{
    m_enabled = false;
    m_hasValidResult = false;
    m_constants = {};

    if (m_depthReductionPass == nullptr)
    {
        return;
    }

    const LightComponent* light =
        findMainShadowCastingDirectionalLight();

    if (light == nullptr)
    {
        return;
    }

    const LightShadowSettings& shadowSettings = light->getData().shadow;
    const GameObject* owner = light->getOwner();
    const Transform* transform =
        owner != nullptr ? owner->GetTransform() : nullptr;

    if (transform == nullptr)
    {
        return;
    }

    Vector3 lightDirection = transform->getForward();

    if (lightDirection.LengthSquared() <= 0.000001f)
    {
        return;
    }

    lightDirection.Normalize();

    m_constants.inverseView = ctx.view.Invert().Transpose();

    m_constants.projection = ctx.projection.Transpose();

    m_constants.lightDirection = lightDirection;
    m_constants.sunDistance = SHADOW_LIGHT_DISTANCE_PADDING;

    m_constants.minOrthoSize = SHADOW_MIN_ORTHO_SIZE;

    m_constants.shadowBias = shadowSettings.shadowBias;

    m_constants.shadowStrength = shadowSettings.shadowStrength;

    m_constants.shadowsEnabled = 1u;

    m_constants.pcfEnabled = shadowSettings.pcfEnabled ? 1u : 0u;

    m_constants.pcfRadius = shadowSettings.pcfEnabled ? shadowSettings.pcfRadius : 0u;

    const uint32_t shadowMapSize = std::max(1u, shadowSettings.shadowMapSize);

    const float inverseShadowMapSize = 1.0f / static_cast<float>(shadowMapSize);

    m_constants.shadowMapTexelSizeX = inverseShadowMapSize;

    m_constants.shadowMapTexelSizeY = inverseShadowMapSize;

    m_constants.paddingSettings = 0.0f;

    m_constants.padding = Vector3::Zero;

    m_enabled = true;
}

void ShadowFrustumComputePass::transitionOutputBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState)
{
    if (commandList == nullptr ||
        m_lightViewProjectionBuffer == nullptr ||
        m_outputBufferState == newState)
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_lightViewProjectionBuffer.Get(),
            m_outputBufferState,
            newState);

    commandList->ResourceBarrier(1, &barrier);

    m_outputBufferState = newState;
}

void ShadowFrustumComputePass::apply(
    ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "ShadowFrustumComputePass");

    m_hasValidResult = false;

    if (commandList == nullptr ||
        !m_enabled ||
        m_depthReductionPass == nullptr ||
        m_lightViewProjectionBuffer == nullptr)
    {
        END_EVENT(commandList);
        return;
    }

    Texture* minMaxTexture =
        m_depthReductionPass->getResultTexture();

    if (minMaxTexture == nullptr ||
        !minMaxTexture->hasSRV())
    {
        END_EVENT(commandList);
        return;
    }

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        app->getModuleDescriptors()
            ->getHeap(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
            .getHeap()
    };

    commandList->SetDescriptorHeaps(
        _countof(descriptorHeaps),
        descriptorHeaps);

    transitionOutputBuffer(
        commandList,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->SetPipelineState(
        m_pipelineState.Get());

    commandList->SetComputeRootSignature(
        m_rootSignature.Get());

    commandList->SetComputeRootDescriptorTable(
        0,
        minMaxTexture->getSRV().gpu);

    commandList->SetComputeRootUnorderedAccessView(
        1,
        m_lightViewProjectionBuffer->GetGPUVirtualAddress());

    commandList->SetComputeRoot32BitConstants(
        2,
        sizeof(FrustumConstants) / sizeof(uint32_t),
        &m_constants,
        0);

    commandList->Dispatch(1, 1, 1);

    CD3DX12_RESOURCE_BARRIER uavBarrier =
        CD3DX12_RESOURCE_BARRIER::UAV(
            m_lightViewProjectionBuffer.Get());

    commandList->ResourceBarrier(1, &uavBarrier);

    transitionOutputBuffer(
        commandList,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    m_hasValidResult = true;

    END_EVENT(commandList);
}

D3D12_GPU_VIRTUAL_ADDRESS
ShadowFrustumComputePass::getLightViewProjectionBufferAddress() const
{
    if (m_lightViewProjectionBuffer == nullptr)
    {
        return 0;
    }

    return m_lightViewProjectionBuffer->GetGPUVirtualAddress();
}