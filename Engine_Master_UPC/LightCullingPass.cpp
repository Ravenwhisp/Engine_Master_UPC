#include "Globals.h"
#include "LightCullingPass.h"

#include "Application.h"
#include "SceneLightingSettings.h"
#include "LightComponent.h"
#include "Transform.h"
#include "Lights.h"
#include "GameObject.h"

#include "ModuleScene.h"
#include "ModuleD3D12.h"

#include "RenderContext.h"
#include "RingBuffer.h"

#include <d3dcompiler.h>
#include "PlatformHelpers.h"

#define MAX_LIGHTS_PER_TILE_PER_TYPE 32

LightCullingPass::LightCullingPass(ComPtr<ID3D12Device4> device)
	: m_device(device)
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[4] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);              // input lights CB
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);              // LightCullingCB
    rootParameters[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);         // depth texture
    rootParameters[3].InitAsUnorderedAccessView(0, 0, D3D12_SHADER_VISIBILITY_ALL);             // output light index array

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
    ThrowIfFailed(D3DReadFileToBlob(L"LightCullingPass.cso", &computeShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void LightCullingPass::prepare(const RenderContext& ctx)
{
    GPULightsConstantBuffer lightsCB{};
    {
        //PERF_RENDER("DeferredShadingPass::prepare::PackLights");
        lightsCB = packLightsForGPU(
            app->getModuleScene()->getLightComponents(),
            m_lighting->ambientColor,
            m_lighting->ambientIntensity);
    }

    {
        //PERF_RENDER("DeferredShadingPass::prepare::UploadLightsCB");
        m_lightsAddress = ctx.ringBuffer->allocate(
            &lightsCB,
            sizeof(GPULightsConstantBuffer),
            app->getModuleD3D12()->getCurrentFrame());
    }

    m_tilesX = (static_cast<uint32_t>(ctx.viewport.Width) + m_tileSize - 1) / m_tileSize;
    m_tilesY = (static_cast<uint32_t>(ctx.viewport.Height) + m_tileSize - 1) / m_tileSize;

    LightCullingCB lightCullingCB{};
    lightCullingCB.view = ctx.view;
    lightCullingCB.projection = ctx.projection;
    lightCullingCB.inverseProjection = ctx.projection.Invert();
    lightCullingCB.screenWidth = static_cast<uint32_t>(ctx.viewport.Width);
    lightCullingCB.screenHeight = static_cast<uint32_t>(ctx.viewport.Height);

    m_lightCullingCBAddress = ctx.ringBuffer->allocate(&lightCullingCB, sizeof(LightCullingCB), app->getModuleD3D12()->getCurrentFrame());

    // Could be optimized by just having an array of buffers with size FRAMES_IN_FLIGHT as to not allocate so many buffers
    m_lightIndexesBufferAddress = ctx.ringBuffer->allocate(nullptr, m_lightIndexCapacity, app->getModuleD3D12()->getCurrentFrame());
}

void LightCullingPass::apply(ID3D12GraphicsCommandList4* commandList) {
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetComputeRootSignature(m_rootSignature.Get());

    commandList->SetComputeRootConstantBufferView(0, m_lightsAddress);
    commandList->SetComputeRootConstantBufferView(1, m_lightCullingCBAddress);
    commandList->SetComputeRootUnorderedAccessView(0, m_lightIndexesBufferAddress);

    // 1 thread group per tile, 1 thread per pixel in tile
    commandList->Dispatch(m_tilesX, m_tilesY, 1);
}

GPULightsConstantBuffer LightCullingPass::packLightsForGPU(
    const std::vector<LightComponent*>& lights,
    const Vector3& ambientColor,
    float ambientIntensity) const
{
    GPULightsConstantBuffer cb{};
    cb.ambientColor = ambientColor;
    cb.ambientIntensity = ambientIntensity;

    for (const LightComponent* light : lights)
    {
        if (!light->isActive())
            continue;

        const GameObject* owner = light->getOwner();
        if (!owner || !owner->IsActiveInWindowHierarchy())
            continue;

        const Transform* transform = owner->GetTransform();
        if (!transform)
            continue;

        const LightData& data = light->getData();
        const LightCommon& common = data.common;
        const Matrix& world = transform->getGlobalMatrix();
        const Vector3      pos(world._41, world._42, world._43);
        const Vector3      fwd = transform->getForward();

        switch (data.type)
        {
        case LightType::DIRECTIONAL:
            if (cb.directionalCount < LightDefaults::MAX_DIRECTIONAL_LIGHTS)
            {
                auto& l = cb.directionalLights[cb.directionalCount++];
                l.direction = fwd;
                l.color = common.color;
                l.intensity = common.intensity;
            }
            break;

        case LightType::POINT:
            if (cb.pointCount < LightDefaults::MAX_POINT_LIGHTS)
            {
                auto& l = cb.pointLights[cb.pointCount++];
                l.position = pos;
                l.radius = data.parameters.point.radius;
                l.color = common.color;
                l.intensity = common.intensity;
            }
            break;

        case LightType::SPOT:
            if (cb.spotCount < LightDefaults::MAX_SPOT_LIGHTS)
            {
                const auto& sp = data.parameters.spot;
                auto& l = cb.spotLights[cb.spotCount++];
                l.position = pos;
                l.direction = fwd;
                l.radius = sp.radius;
                l.color = common.color;
                l.intensity = common.intensity;
                l.cosineInnerAngle = std::cos(XMConvertToRadians(sp.innerAngleDegrees));
                l.cosineOuterAngle = std::cos(XMConvertToRadians(sp.outerAngleDegrees));
                l.boundingSphere = calculateBoundingSphere(l);
            }
            break;

        default: break;
        }
    }

    return cb;
}

Vector4 LightCullingPass::calculateBoundingSphere(GPUSpotLight& l) const {
    Vector4 sphere;

    float halfAngle = acosf(l.cosineOuterAngle);
    float spotRadius = l.radius;

    if (halfAngle > IM_PI / 4) {
        float radius = spotRadius * tan(halfAngle);
        Vector3 center = l.position + l.direction * spotRadius;
        return Vector4(center.x, center.y, center.z, radius);
    }

    float cosAngle = cos(halfAngle);
    float radius = spotRadius * 0.5f / (cosAngle * cosAngle);
    Vector3 center = l.position + l.direction * radius;
    return Vector4(center.x, center.y, center.z, radius);
}

void LightCullingPass::resize(uint32_t width, uint32_t height) 
{
    m_tilesX = (width + m_tileSize - 1) / m_tileSize;
    m_tilesY = (height + m_tileSize - 1) / m_tileSize;
    const uint32_t tileCount = m_tilesX * m_tilesY;
    m_lightIndexCapacity = tileCount * MAX_LIGHTS_PER_TILE_PER_TYPE * 2;
}