#include "Globals.h"
#include "VolumetricFogComputePass.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleScene.h"
#include "ModuleDescriptors.h"

#include "RenderContext.h"
#include "Scene.h"
#include "Texture.h"
#include "VolumetricFogSettings.h"
#include "VolumetricFogTypes.h"
#include "GameObject.h"
#include "LightComponent.h"
#include "Lights.h"
#include "Transform.h"


#include "PlatformHelpers.h"
#include <d3dcompiler.h>
#include <cmath>


namespace
{
    bool getCameraNearDistance(const Matrix& projection, float& nearDistance)
    {
        if (std::abs(projection._33) <= 0.000001f) return false;

        nearDistance = std::abs(projection._43 / projection._33);
        return nearDistance > 0.0f;
    }
}

VolumetricFogComputePass::VolumetricFogComputePass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    createMediumRootSignature();
    createMediumPipelineState();
    createLightingRootSignature();
    createLightingPipelineState();
}

VolumetricFogComputePass::~VolumetricFogComputePass() = default;

void VolumetricFogComputePass::prepare(const RenderContext& ctx)
{
    m_enabled = false;
    m_gridConstants = {};

    Scene* scene = app->getModuleScene()->getScene();
    if (scene == nullptr) return;

    const VolumetricFogSettings& settings = scene->getVolumetricFogSettings();
    if (!settings.enabled) return;

    m_mediumConstants = {};
    m_mediumConstants.density = settings.density;
    m_mediumConstants.scatteringCoefficient = settings.scatteringCoefficient;
    m_mediumConstants.extinctionCoefficient = settings.extinctionCoefficient;
    m_mediumConstants.gridWidth = VolumetricFog::GRID_WIDTH;
    m_mediumConstants.gridHeight = VolumetricFog::GRID_HEIGHT;
    m_mediumConstants.gridDepth = VolumetricFog::GRID_DEPTH;

    m_lightingConstants = {};
    m_lightingConstants.inverseView = m_gridConstants.inverseView;
    m_lightingConstants.projectionScale = m_gridConstants.projectionScale;
    m_lightingConstants.nearDistance = m_gridConstants.nearDistance;
    m_lightingConstants.maxDistance = m_gridConstants.maxDistance;
    m_lightingConstants.cameraPosition = ctx.cameraPosition;
    m_lightingConstants.anisotropy = settings.anisotropy;
    m_lightingConstants.gridWidth = VolumetricFog::GRID_WIDTH;
    m_lightingConstants.gridHeight = VolumetricFog::GRID_HEIGHT;
    m_lightingConstants.gridDepth = VolumetricFog::GRID_DEPTH;

    const LightComponent* directionalLight = findVolumetricDirectionalLight();

    if (directionalLight != nullptr)
    {
        const Transform* transform = directionalLight->getOwner()->GetTransform();
        const LightData& lightData = directionalLight->getData();

        Vector3 direction = transform->getForward();
        if (direction.LengthSquared() > 0.000001f) direction.Normalize();

        m_lightingConstants.lightDirection = direction;
        m_lightingConstants.lightColor = lightData.common.color;
        m_lightingConstants.lightIntensity = lightData.common.intensity;
        m_lightingConstants.hasDirectionalLight = 1;
    }

    float nearDistance = 0.0f;
    if (!getCameraNearDistance(ctx.projection, nearDistance)) return;

    const float maxDistance = std::max(settings.maxDistance, nearDistance + VolumetricFog::MIN_DEPTH_RANGE);
    const float projectionScaleX = ctx.projection._11;
    const float projectionScaleY = ctx.projection._22;

    if (std::abs(projectionScaleX) <= 0.000001f || std::abs(projectionScaleY) <= 0.000001f) return;

    ensureVolumes();

    m_gridConstants.inverseView = ctx.view.Invert().Transpose();
    m_gridConstants.projectionScale = Vector2(projectionScaleX, projectionScaleY);
    m_gridConstants.nearDistance = nearDistance;
    m_gridConstants.maxDistance = maxDistance;
    m_gridConstants.gridWidth = VolumetricFog::GRID_WIDTH;
    m_gridConstants.gridHeight = VolumetricFog::GRID_HEIGHT;
    m_gridConstants.gridDepth = VolumetricFog::GRID_DEPTH;

    m_enabled = m_mediumVolume != nullptr && m_lightingVolume != nullptr && m_integratedVolume != nullptr;
}

void VolumetricFogComputePass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (commandList == nullptr || !m_enabled || m_mediumVolume == nullptr || m_lightingVolume == nullptr || m_mediumPipelineState == nullptr || m_mediumRootSignature == nullptr || m_lightingPipelineState == nullptr || m_lightingRootSignature == nullptr) return;

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    transitionVolume(commandList, m_mediumVolume.get(), m_mediumVolumeState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    transitionVolume(commandList, m_lightingVolume.get(), m_lightingVolumeState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    BEGIN_EVENT(commandList, "VolumetricFog::InjectMedium");

    commandList->SetPipelineState(m_mediumPipelineState.Get());
    commandList->SetComputeRootSignature(m_mediumRootSignature.Get());
    commandList->SetComputeRoot32BitConstants(0, sizeof(VolumetricFog::MediumConstants) / sizeof(uint32_t), &m_mediumConstants, 0);
    commandList->SetComputeRootDescriptorTable(1, m_mediumVolume->getUAV().gpu);

    const uint32_t groupCountX = (VolumetricFog::GRID_WIDTH + VolumetricFog::INJECT_GROUP_SIZE_X - 1) / VolumetricFog::INJECT_GROUP_SIZE_X;
    const uint32_t groupCountY = (VolumetricFog::GRID_HEIGHT + VolumetricFog::INJECT_GROUP_SIZE_Y - 1) / VolumetricFog::INJECT_GROUP_SIZE_Y;
    const uint32_t groupCountZ = (VolumetricFog::GRID_DEPTH + VolumetricFog::INJECT_GROUP_SIZE_Z - 1) / VolumetricFog::INJECT_GROUP_SIZE_Z;

    commandList->Dispatch(groupCountX, groupCountY, groupCountZ);

    END_EVENT(commandList);

    transitionVolume(commandList, m_mediumVolume.get(), m_mediumVolumeState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    BEGIN_EVENT(commandList, "VolumetricFog::DirectionalLighting");

    commandList->SetPipelineState(m_lightingPipelineState.Get());
    commandList->SetComputeRootSignature(m_lightingRootSignature.Get());
    commandList->SetComputeRoot32BitConstants(0, sizeof(VolumetricFog::LightingConstants) / sizeof(uint32_t), &m_lightingConstants, 0);
    commandList->SetComputeRootDescriptorTable(1, m_mediumVolume->getSRV().gpu);
    commandList->SetComputeRootDescriptorTable(2, m_lightingVolume->getUAV().gpu);

    const uint32_t lightingGroupsX = (VolumetricFog::GRID_WIDTH + VolumetricFog::LIGHTING_GROUP_SIZE_X - 1) / VolumetricFog::LIGHTING_GROUP_SIZE_X;
    const uint32_t lightingGroupsY = (VolumetricFog::GRID_HEIGHT + VolumetricFog::LIGHTING_GROUP_SIZE_Y - 1) / VolumetricFog::LIGHTING_GROUP_SIZE_Y;
    const uint32_t lightingGroupsZ = (VolumetricFog::GRID_DEPTH + VolumetricFog::LIGHTING_GROUP_SIZE_Z - 1) / VolumetricFog::LIGHTING_GROUP_SIZE_Z;

    commandList->Dispatch(lightingGroupsX, lightingGroupsY, lightingGroupsZ);

    END_EVENT(commandList);
}

void VolumetricFogComputePass::createMediumRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE uavRange;
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].InitAsConstants(sizeof(VolumetricFog::MediumConstants) / sizeof(uint32_t), 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_mediumRootSignature)));
}

void VolumetricFogComputePass::createMediumPipelineState()
{
    ComPtr<ID3DBlob> computeShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VolumetricFogInjectMediumCS.cso", &computeShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_mediumRootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_mediumPipelineState)));
}

void VolumetricFogComputePass::createLightingRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE uavRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    rootParameters[0].InitAsConstants(sizeof(VolumetricFog::LightingConstants) / sizeof(uint32_t), 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_lightingRootSignature)));
}

void VolumetricFogComputePass::createLightingPipelineState()
{
    ComPtr<ID3DBlob> computeShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VolumetricFogLightingCS.cso", &computeShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_lightingRootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_lightingPipelineState)));
}

void VolumetricFogComputePass::ensureVolumes()
{
    if (m_mediumVolume == nullptr)
    {
        m_mediumVolume.reset(app->getModuleResources()->createVolumeTexture(
                VolumetricFog::GRID_WIDTH,
                VolumetricFog::GRID_HEIGHT,
                VolumetricFog::GRID_DEPTH,
                DXGI_FORMAT_R16G16B16A16_FLOAT));

        m_mediumVolume->setName(L"VolumetricFog_Medium");
    }

    if (m_lightingVolume == nullptr)
    {
        m_lightingVolume.reset(app->getModuleResources()->createVolumeTexture(
                VolumetricFog::GRID_WIDTH,
                VolumetricFog::GRID_HEIGHT,
                VolumetricFog::GRID_DEPTH,
                DXGI_FORMAT_R16G16B16A16_FLOAT));

        m_lightingVolume->setName(L"VolumetricFog_Lighting");
    }

    if (m_integratedVolume == nullptr)
    {
        m_integratedVolume.reset(app->getModuleResources()->createVolumeTexture(
                VolumetricFog::GRID_WIDTH,
                VolumetricFog::GRID_HEIGHT,
                VolumetricFog::GRID_DEPTH,
                DXGI_FORMAT_R16G16B16A16_FLOAT));

        m_integratedVolume->setName(L"VolumetricFog_Integrated");
    }
}

const LightComponent* VolumetricFogComputePass::findVolumetricDirectionalLight() const
{
    const std::vector<LightComponent*>& lights = app->getModuleScene()->getLightComponents();
    const LightComponent* fallback = nullptr;

    for (const LightComponent* light : lights)
    {
        if (light == nullptr || !light->isActive()) continue;

        const GameObject* owner = light->getOwner();
        if (owner == nullptr || !owner->IsActiveInWindowHierarchy()) continue;

        const LightData& data = light->getData();
        if (data.type != LightType::DIRECTIONAL) continue;

        if (fallback == nullptr) fallback = light;
        if (data.shadow.castShadows) return light;
    }

    return fallback;
}

void VolumetricFogComputePass::transitionVolume(ID3D12GraphicsCommandList4* commandList, Texture* texture, D3D12_RESOURCE_STATES& currentState, D3D12_RESOURCE_STATES newState)
{
    if (texture == nullptr || currentState == newState) return;

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture->getD3D12Resource().Get(), currentState, newState);
    commandList->ResourceBarrier(1, &barrier);
    currentState = newState;
}