#include "Globals.h"
#include "ShadowFrustumComputePass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "ModuleScene.h"
#include "RingBuffer.h"

#include "DepthReductionPass.h"
#include "RenderContext.h"
#include "Texture.h"

#include "LightComponent.h"
#include "Lights.h"
#include "GameObject.h"
#include "Transform.h"
#include "Scene.h"
#include "CameraComponent.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include "PlatformHelpers.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
    constexpr bool SHADOW_DEBUG_DEPTH_ENABLED = false;

    bool buildClipVolumeCorners(const Matrix& viewProjection, Vector3 corners[8])
    {
        const Matrix inverseViewProjection = viewProjection.Invert();

        const Vector4 clipCorners[8] =
        {
            Vector4(-1.0f,  1.0f, 0.0f, 1.0f),
            Vector4(1.0f,  1.0f, 0.0f, 1.0f),
            Vector4(1.0f, -1.0f, 0.0f, 1.0f),
            Vector4(-1.0f, -1.0f, 0.0f, 1.0f),

            Vector4(-1.0f,  1.0f, 1.0f, 1.0f),
            Vector4(1.0f,  1.0f, 1.0f, 1.0f),
            Vector4(1.0f, -1.0f, 1.0f, 1.0f),
            Vector4(-1.0f, -1.0f, 1.0f, 1.0f)
        };

        for (uint32_t i = 0; i < 8; ++i)
        {
            Vector4 worldPoint = Vector4::Transform(clipCorners[i], inverseViewProjection);

            if (std::abs(worldPoint.w) <= 0.000001f)
            {
                return false;
            }

            worldPoint /= worldPoint.w;
            corners[i] = Vector3(worldPoint.x, worldPoint.y, worldPoint.z);
        }

        return true;
    }

    void drawWireBox(const Vector3 corners[8], const float color[3])
    {
        dd::line(&corners[0].x, &corners[1].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[1].x, &corners[2].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[2].x, &corners[3].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[3].x, &corners[0].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);

        dd::line(&corners[4].x, &corners[5].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[5].x, &corners[6].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[6].x, &corners[7].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[7].x, &corners[4].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);

        dd::line(&corners[0].x, &corners[4].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[1].x, &corners[5].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[2].x, &corners[6].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
        dd::line(&corners[3].x, &corners[7].x, color, 0, SHADOW_DEBUG_DEPTH_ENABLED);
    }

    void drawAabbFromPoints(const Vector3 points[8], const float color[3])
    {
        Vector3 minPoint = points[0];
        Vector3 maxPoint = points[0];

        for (uint32_t i = 1; i < 8; ++i)
        {
            minPoint.x = std::min(minPoint.x, points[i].x); minPoint.y = std::min(minPoint.y, points[i].y); minPoint.z = std::min(minPoint.z, points[i].z);
            maxPoint.x = std::max(maxPoint.x, points[i].x); maxPoint.y = std::max(maxPoint.y, points[i].y); maxPoint.z = std::max(maxPoint.z, points[i].z);
        }

        Vector3 corners[8] =
        {
            { minPoint.x, maxPoint.y, minPoint.z }, { maxPoint.x, maxPoint.y, minPoint.z },
            { maxPoint.x, minPoint.y, minPoint.z }, { minPoint.x, minPoint.y, minPoint.z },
            { minPoint.x, maxPoint.y, maxPoint.z }, { maxPoint.x, maxPoint.y, maxPoint.z },
            { maxPoint.x, minPoint.y, maxPoint.z }, { minPoint.x, minPoint.y, maxPoint.z }
        };

        drawWireBox(corners, color);
    }

    bool buildCameraSubFrustumCorners(const Matrix& view, const Matrix& projection, float nearDistance, float farDistance, Vector3 corners[8])
    {
        if (nearDistance <= 0.0f || farDistance <= nearDistance)
        {
            return false;
        }

        const float xScale = projection._11;
        const float yScale = projection._22;

        if (std::abs(xScale) <= 0.000001f || std::abs(yScale) <= 0.000001f)
        {
            return false;
        }

        const Matrix inverseView = view.Invert();

        const float ndcX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
        const float ndcY[4] = { 1.0f, 1.0f, -1.0f, -1.0f };

        for (uint32_t depthIndex = 0; depthIndex < 2; ++depthIndex)
        {
            const float distance = depthIndex == 0 ? nearDistance : farDistance;

            for (uint32_t cornerIndex = 0; cornerIndex < 4; ++cornerIndex)
            {
                Vector3 viewPoint;
                viewPoint.x = ndcX[cornerIndex] * distance / xScale;
                viewPoint.y = ndcY[cornerIndex] * distance / yScale;
                viewPoint.z = -distance;

                corners[depthIndex * 4 + cornerIndex] = Vector3::Transform(viewPoint, inverseView);
            }
        }

        return true;
    }

    bool getCameraDepthRange(const Matrix& projection, float& nearDistance, float& farDistance)
    {
        const float nearDenominator = projection._33;
        const float farDenominator = 1.0f + projection._33;

        if (std::abs(nearDenominator) <= 0.000001f || std::abs(farDenominator) <= 0.000001f)
        {
            return false;
        }

        nearDistance = projection._43 / nearDenominator;
        farDistance = projection._43 / farDenominator;

        nearDistance = std::abs(nearDistance);
        farDistance = std::abs(farDistance);

        if (farDistance < nearDistance)
        {
            std::swap(nearDistance, farDistance);
        }

        return nearDistance > 0.0f && farDistance > nearDistance;
    }

}

ShadowFrustumComputePass::ShadowFrustumComputePass(ComPtr<ID3D12Device4> device, DepthReductionPass* depthReductionPass)
    : m_device(device)
    , m_depthReductionPass(depthReductionPass)
{
    createRootSignature();
    createPipelineState();
    createOutputBuffer();
    createDebugReadbackBuffers();
}

void ShadowFrustumComputePass::createRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE minMaxRange;
    minMaxRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};

    // t0: final 1x1 min/max depth texture
    rootParameters[0].InitAsDescriptorTable(1, &minMaxRange, D3D12_SHADER_VISIBILITY_ALL);

    // u0: output shadow data buffer
    rootParameters[1].InitAsUnorderedAccessView(0, 0);

    // b0: inverse view, projection, light direction and fitting settings
    rootParameters[2].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void ShadowFrustumComputePass::createPipelineState()
{
    ComPtr<ID3DBlob> computeShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"ShadowFrustumComputeShader.cso", &computeShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void ShadowFrustumComputePass::createOutputBuffer()
{
    constexpr size_t ALIGNMENT = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    constexpr size_t BUFFER_SIZE = ((sizeof(ShadowDataCB) + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;

    static_assert(BUFFER_SIZE == 512, "ShadowDataBuffer must be large enough for cascaded shadow data.");

    for (uint32_t i = 0; i < SHADOW_VIEW_COUNT; ++i)
    {
        const char* name = i == 0 ? "ShadowDataBuffer_Editor" : "ShadowDataBuffer_Game";

        m_shadowDataBuffers[i] = app->getModuleResources()->createDefaultBuffer(
            BUFFER_SIZE,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            name);

        m_outputBufferStates[i] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
}

void ShadowFrustumComputePass::createDebugReadbackBuffers()
{
    m_debugReadbackBuffers.resize(FRAMES_IN_FLIGHT);
    m_debugReadbackPending.assign(FRAMES_IN_FLIGHT, false);
    m_debugCaptureMetadata.resize(FRAMES_IN_FLIGHT);

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_READBACK);
    const CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ShadowDataCB));

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        DXCall(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_debugReadbackBuffers[i])));

        m_debugReadbackBuffers[i]->SetName(L"ShadowCascadeDebugReadback");
    }
}

void ShadowFrustumComputePass::refreshDebugReadbackForCurrentFrame()
{
    ModuleD3D12* d3d12 = app->getModuleD3D12();

    if (d3d12 == nullptr || d3d12->getCommandQueue() == nullptr)
    {
        return;
    }

    const uint32_t frameIndex = d3d12->getCurrentFrameIndex();

    if (frameIndex >= m_debugReadbackBuffers.size())
    {
        return;
    }

    const uint64_t frameFenceValue = d3d12->getCurrentFrame();

    // prepare() can be called more than once per engine frame because Editor
    // and Game use the same render pipeline. Only inspect this frame slot once.
    if (m_hasObservedDebugFrame &&
        m_observedDebugFrameIndex == frameIndex &&
        m_observedDebugFrameFenceValue == frameFenceValue)
    {
        return;
    }

    m_observedDebugFrameIndex = frameIndex;
    m_observedDebugFrameFenceValue = frameFenceValue;
    m_hasObservedDebugFrame = true;

    if (!m_debugReadbackPending[frameIndex])
    {
        return;
    }

    if (!d3d12->getCommandQueue()->isFenceComplete(frameFenceValue))
    {
        return;
    }

    ComPtr<ID3D12Resource>& readbackBuffer = m_debugReadbackBuffers[frameIndex];

    if (readbackBuffer == nullptr)
    {
        return;
    }

    void* mappedData = nullptr;

    D3D12_RANGE readRange{};
    readRange.Begin = 0;
    readRange.End = sizeof(ShadowDataCB);

    const HRESULT mapResult = readbackBuffer->Map(0, &readRange, &mappedData);

    if (FAILED(mapResult) || mappedData == nullptr)
    {
        return;
    }

    std::memcpy(&m_debugShadowData, mappedData, sizeof(ShadowDataCB));

    D3D12_RANGE writeRange{};
    writeRange.Begin = 0;
    writeRange.End = 0;

    readbackBuffer->Unmap(0, &writeRange);

    const DebugCaptureMetadata& metadata = m_debugCaptureMetadata[frameIndex];

    if (metadata.valid)
    {
        m_debugCameraView = metadata.view;
        m_debugCameraProjection = metadata.projection;
        m_hasDebugShadowData = m_debugShadowData.shadowsEnabled != 0;
    }
    else
    {
        m_hasDebugShadowData = false;
    }

    m_debugReadbackPending[frameIndex] = false;
    m_debugCaptureMetadata[frameIndex] = {};
}

void ShadowFrustumComputePass::recordDebugReadback(ID3D12GraphicsCommandList4* commandList)
{
    if (commandList == nullptr || !m_captureDebugReadback || m_currentShadowView >= SHADOW_VIEW_COUNT || m_shadowDataBuffers[m_currentShadowView] == nullptr)
    {
        return;
    }

    ModuleD3D12* d3d12 = app->getModuleD3D12();

    if (d3d12 == nullptr)
    {
        return;
    }

    const uint32_t frameIndex = d3d12->getCurrentFrameIndex();

    if (frameIndex >= m_debugReadbackBuffers.size() ||
        m_debugReadbackBuffers[frameIndex] == nullptr ||
        m_debugReadbackPending[frameIndex])
    {
        return;
    }

    transitionOutputBuffer(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

    commandList->CopyBufferRegion(m_debugReadbackBuffers[frameIndex].Get(), 0, m_shadowDataBuffers[m_currentShadowView].Get(), 0, sizeof(ShadowDataCB));

    m_debugReadbackPending[frameIndex] = true;
}

const LightComponent* ShadowFrustumComputePass::findMainShadowCastingDirectionalLight() const
{
    const std::vector<LightComponent*>& lights = app->getModuleScene()->getLightComponents();

    for (const LightComponent* light : lights)
    {
        if (light == nullptr || !light->isActive())
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

void ShadowFrustumComputePass::prepare(const RenderContext& ctx)
{
    refreshDebugReadbackForCurrentFrame();

    m_constantsAddress = 0;
    m_enabled = false;
    m_hasValidResult = false;
    m_captureDebugReadback = false;
    m_drawDebugForCurrentView = false;
    m_constants = {};

    if (m_depthReductionPass == nullptr)
    {
        return;
    }

    const LightComponent* light = findMainShadowCastingDirectionalLight();

    if (light == nullptr)
    {
        m_cascadeUpdateHistory[0].valid = false;
        m_cascadeUpdateHistory[1].valid = false;
        return;
    }

    const LightShadowSettings& shadowSettings = light->getData().shadow;

    const uint64_t frameNumber = app->getModuleD3D12()->getFrameNumber();
    const uint32_t cascadeCount = std::clamp(shadowSettings.cascadeCount, 1u, MAX_SHADOW_CASCADES);

    m_currentShadowView = ctx.viewType == RenderViewType::Game ? 1u : 0u;
    CascadeUpdateHistory& history = m_cascadeUpdateHistory[m_currentShadowView];

    const bool cascadeConfigChanged =
        !history.valid ||
        history.shadowMapSize != shadowSettings.shadowMapSize ||
        history.cascadeCount != cascadeCount ||
        history.cascadeFitMode != static_cast<uint32_t>(shadowSettings.cascadeFitMode) ||
        history.cascadeSplit0 != shadowSettings.cascadeSplit0 ||
        history.cascadeSplit1 != shadowSettings.cascadeSplit1 ||
        history.cascadeSplit2 != shadowSettings.cascadeSplit2;

    static constexpr uint32_t CASCADE_UPDATE_INTERVALS[MAX_SHADOW_CASCADES] = { 1u, 1u, 1u, 1u }; 

    m_cascadeUpdateMask = 0;

    for (uint32_t i = 0; i < cascadeCount; ++i)
    {
        if (frameNumber % CASCADE_UPDATE_INTERVALS[i] == 0) m_cascadeUpdateMask |= 1u << i;
    }

    if (cascadeConfigChanged) m_cascadeUpdateMask = (1u << cascadeCount) - 1u;

    history.shadowMapSize = shadowSettings.shadowMapSize;
    history.cascadeCount = cascadeCount;
    history.cascadeFitMode = static_cast<uint32_t>(shadowSettings.cascadeFitMode);
    history.cascadeSplit0 = shadowSettings.cascadeSplit0;
    history.cascadeSplit1 = shadowSettings.cascadeSplit1;
    history.cascadeSplit2 = shadowSettings.cascadeSplit2;

    m_captureDebugReadback = ctx.viewType == RenderViewType::Game && shadowSettings.cascadeDebugEnabled;

    if (m_captureDebugReadback)
    {
        ModuleD3D12* d3d12 = app->getModuleD3D12();

        if (d3d12 != nullptr)
        {
            const uint32_t frameIndex = d3d12->getCurrentFrameIndex();

            if (frameIndex < m_debugCaptureMetadata.size() && !m_debugReadbackPending[frameIndex])
            {
                m_debugCaptureMetadata[frameIndex].view = ctx.view;
                m_debugCaptureMetadata[frameIndex].projection = ctx.projection;
                m_debugCaptureMetadata[frameIndex].valid = true;
            }
        }
    }

    const GameObject* owner = light->getOwner();
    const Transform* transform = owner != nullptr ? owner->GetTransform() : nullptr;

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
    m_constants.view = ctx.view.Transpose();
    m_constants.projection = ctx.projection.Transpose();

    m_constants.lightDirection = lightDirection;
    m_constants.sunDistance = SHADOW_LIGHT_DISTANCE_PADDING;

    m_constants.shadowBias = shadowSettings.shadowBias;
    m_constants.shadowStrength = shadowSettings.shadowStrength;
    m_constants.pcfEnabled = shadowSettings.pcfEnabled ? 1u : 0u;
    m_constants.pcfRadius = shadowSettings.pcfEnabled ? shadowSettings.pcfRadius : 0u;

    const uint32_t shadowMapSize = std::max(1u, shadowSettings.shadowMapSize);
    m_constants.shadowMapTexelSize = 1.0f / static_cast<float>(shadowMapSize);

    m_constants.cascadeCount = cascadeCount;
    m_constants.cascadeFitMode = static_cast<uint32_t>(shadowSettings.cascadeFitMode);

    m_constants.cascadeSplit0 = shadowSettings.cascadeSplit0;
    m_constants.cascadeSplit1 = shadowSettings.cascadeSplit1;
    m_constants.cascadeSplit2 = shadowSettings.cascadeSplit2;

    m_constants.cascadeUpdateMask = m_cascadeUpdateMask;

    m_constants.cascadeDebugEnabled = ctx.viewType == RenderViewType::Game && shadowSettings.cascadeDebugEnabled ? 1u : 0u;

    m_constants.shadowLightIndex = 0;

    uint32_t directionalIndex = 0;
    for (const LightComponent* candidate : app->getModuleScene()->getLightComponents())
    {
        if (!candidate || !candidate->isActive() || !candidate->getOwner() ||
            !candidate->getOwner()->IsActiveInWindowHierarchy() || !candidate->getOwner()->GetTransform())
            continue;

        if (candidate->getData().type != LightType::DIRECTIONAL)
            continue;

        if (candidate == light)
        {
            m_constants.shadowLightIndex = directionalIndex;
            break;
        }

        ++directionalIndex;
    }

    if (ctx.ringBuffer == nullptr)
    {
        return;
    }

    m_constantsAddress = ctx.ringBuffer->allocate(&m_constants, sizeof(FrustumConstants));

    if (m_constantsAddress == 0)
    {
        return;
    }

    m_enabled = true;
}

void ShadowFrustumComputePass::transitionOutputBuffer(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState)
{
    if (commandList == nullptr || m_currentShadowView >= SHADOW_VIEW_COUNT) return;

    ComPtr<ID3D12Resource>& buffer = m_shadowDataBuffers[m_currentShadowView];
    D3D12_RESOURCE_STATES& state = m_outputBufferStates[m_currentShadowView];

    if (buffer == nullptr || state == newState) return;

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), state, newState);
    commandList->ResourceBarrier(1, &barrier);

    state = newState;
}

void ShadowFrustumComputePass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "ShadowFrustumComputePass");

    m_hasValidResult = false;

    if (commandList == nullptr ||
        !m_enabled ||
        m_depthReductionPass == nullptr ||
        m_currentShadowView >= SHADOW_VIEW_COUNT ||
        m_shadowDataBuffers[m_currentShadowView] == nullptr)
    {
        END_EVENT(commandList);
        return;
    }

    Texture* minMaxTexture = m_depthReductionPass->getResultTexture();

    if (minMaxTexture == nullptr || !minMaxTexture->hasSRV())
    {
        END_EVENT(commandList);
        return;
    }

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    transitionOutputBuffer(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetComputeRootSignature(m_rootSignature.Get());

    commandList->SetComputeRootDescriptorTable(0, minMaxTexture->getSRV().gpu);
    commandList->SetComputeRootUnorderedAccessView(1, m_shadowDataBuffers[m_currentShadowView]->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(2, m_constantsAddress);

    commandList->Dispatch(1, 1, 1);

    CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_shadowDataBuffers[m_currentShadowView].Get());
    commandList->ResourceBarrier(1, &uavBarrier);

    recordDebugReadback(commandList);

    transitionOutputBuffer(commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    m_hasValidResult = true;
    m_cascadeUpdateHistory[m_currentShadowView].valid = true;

    END_EVENT(commandList);
}

void ShadowFrustumComputePass::debugDraw()
{
    if (!m_drawDebugForCurrentView || !m_hasDebugShadowData) return;

    const float fittedNearDistance = m_debugShadowData.cascadePadding.y;
    const uint32_t activeCascadeCount = std::clamp(m_debugShadowData.cascadeCount, 1u, MAX_SHADOW_CASCADES);

    const float cascadeFarDistances[MAX_SHADOW_CASCADES] =
    {
        m_debugShadowData.cascadeFarDistances.x,
        m_debugShadowData.cascadeFarDistances.y,
        m_debugShadowData.cascadeFarDistances.z,
        m_debugShadowData.cascadeFarDistances.w
    };

    const float fittedFarDistance = cascadeFarDistances[activeCascadeCount - 1];
    if (fittedNearDistance <= 0.0f || fittedFarDistance <= fittedNearDistance) return;

    const float cascadeColors[MAX_SHADOW_CASCADES][3] =
    {
        { 1.0f, 0.2f, 0.2f },
        { 0.2f, 1.0f, 0.2f },
        { 0.2f, 0.4f, 1.0f },
        { 1.0f, 0.8f, 0.2f }
    };

    Vector3 cameraCorners[8];

    if (buildCameraSubFrustumCorners(m_debugCameraView, m_debugCameraProjection, fittedNearDistance, fittedFarDistance, cameraCorners))
    {
        const float cameraColor[3] = { 1.0f, 1.0f, 1.0f };
        drawAabbFromPoints(cameraCorners, cameraColor);
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < activeCascadeCount; ++cascadeIndex)
    {
        float cascadeNearDistance = fittedNearDistance;

        if (m_debugShadowData.cascadeFitMode == static_cast<uint32_t>(ShadowCascadeFitMode::FIT_TO_CASCADE) && cascadeIndex > 0)
            cascadeNearDistance = cascadeFarDistances[cascadeIndex - 1];

        const float cascadeFarDistance = cascadeFarDistances[cascadeIndex];

        Vector3 cascadeCorners[8];
        if (buildCameraSubFrustumCorners(m_debugCameraView, m_debugCameraProjection, cascadeNearDistance, cascadeFarDistance, cascadeCorners))
            drawAabbFromPoints(cascadeCorners, cascadeColors[cascadeIndex]);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowFrustumComputePass::getShadowDataBufferAddress() const
{
    if (m_currentShadowView >= SHADOW_VIEW_COUNT || m_shadowDataBuffers[m_currentShadowView] == nullptr) return 0;

    return m_shadowDataBuffers[m_currentShadowView]->GetGPUVirtualAddress();
}