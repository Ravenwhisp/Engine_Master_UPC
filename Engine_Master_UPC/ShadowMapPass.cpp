#include "Globals.h"
#include "ShadowMapPass.h"
#include "ShadowCascadeResolution.h"

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

namespace
{
    constexpr D3D12_RESOURCE_STATES CASCADE_SHADER_RESOURCE_STATE =
        static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);


    bool buildConservativeLightViewProjection(const RenderContext& ctx, const Vector3& lightDirection, float sunDistance, Matrix& lightViewProjection)
    {
        const Matrix inverseViewProjection = (ctx.view * ctx.projection).Invert();

        const Vector4 clipCorners[8] =
        {
            Vector4(-1.0f, -1.0f, 0.0f, 1.0f), Vector4(1.0f, -1.0f, 0.0f, 1.0f),
            Vector4(-1.0f,  1.0f, 0.0f, 1.0f), Vector4(1.0f,  1.0f, 0.0f, 1.0f),
            Vector4(-1.0f, -1.0f, 1.0f, 1.0f), Vector4(1.0f, -1.0f, 1.0f, 1.0f),
            Vector4(-1.0f,  1.0f, 1.0f, 1.0f), Vector4(1.0f,  1.0f, 1.0f, 1.0f)
        };

        Vector3 worldCorners[8];
        Vector3 center = Vector3::Zero;

        for (uint32_t i = 0; i < 8; ++i)
        {
            Vector4 world = Vector4::Transform(clipCorners[i], inverseViewProjection);
            if (std::abs(world.w) <= 0.000001f) return false;

            world /= world.w;
            worldCorners[i] = Vector3(world.x, world.y, world.z);
            center += worldCorners[i];
        }

        center /= 8.0f;

        float radius = 0.0f;
        for (const Vector3& corner : worldCorners) radius = std::max(radius, Vector3::Distance(center, corner));

        if (radius <= 0.0001f) return false;

        Vector3 direction = lightDirection;
        direction.Normalize();

        const Vector3 up = std::abs(direction.y) > 0.95f ? Vector3::Forward : Vector3::Up;
        const Vector3 eye = center - direction * (radius + sunDistance);

        const Matrix lightView = Matrix::CreateLookAt(eye, center, up);
        const Matrix lightProjection = Matrix::CreateOrthographic(radius * 2.0f, radius * 2.0f, 0.0f, radius * 2.0f + sunDistance);

        lightViewProjection = lightView * lightProjection;
        return true;
    }

    bool intersectsLightFrustum(const MeshRenderer& renderer, const Matrix& lightViewProjection)
    {
        if (renderer.getSkin() != nullptr) return true;

        const Vector3* points = renderer.getBoundingBox().getPoints();

        bool outsideLeft = true;
        bool outsideRight = true;
        bool outsideBottom = true;
        bool outsideTop = true;
        bool outsideNear = true;
        bool outsideFar = true;

        for (uint32_t i = 0; i < 8; ++i)
        {
            const Vector4 clip = Vector4::Transform(Vector4(points[i].x, points[i].y, points[i].z, 1.0f), lightViewProjection);

            outsideLeft &= clip.x < -clip.w;
            outsideRight &= clip.x > clip.w;
            outsideBottom &= clip.y < -clip.w;
            outsideTop &= clip.y > clip.w;
            outsideNear &= clip.z < 0.0f;
            outsideFar &= clip.z > clip.w;
        }

        return !(outsideLeft || outsideRight || outsideBottom || outsideTop || outsideNear || outsideFar);
    }

}

ShadowMapPass::ShadowMapPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass)
    : m_device(device)
    , m_shadowFrustumComputePass(shadowFrustumComputePass)
{
    createCascadeShadowMap(1u, 1u);
    createRootSignature();
    createPipelineState();
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

void ShadowMapPass::createCascadeShadowMap(uint32_t size, uint32_t cascadeCount)
{
    cascadeCount = std::clamp(cascadeCount, 1u, MAX_SHADOW_CASCADES);

    m_currentCascadeShadowMapSize = size;
    m_currentCascadeCount = cascadeCount;

    for (uint32_t viewIndex = 0; viewIndex < SHADOW_VIEW_COUNT; ++viewIndex)
    {
        for (uint32_t cascadeIndex = 0; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex)
        {
            if (cascadeIndex < cascadeCount)
            {
                const uint32_t resolution = std::max(1u, size / SHADOW_CASCADE_DIVISOR(cascadeIndex));

                m_cascadeShadowMaps[viewIndex][cascadeIndex].reset(app->getModuleResources()->createShadowMap(resolution));

                const std::wstring viewName = viewIndex == 0 ? L"Editor" : L"Game";
                m_cascadeShadowMaps[viewIndex][cascadeIndex]->setName(L"ShadowCascade_" + viewName + L"_" + std::to_wstring(cascadeIndex));

                m_cascadeShadowMapStates[viewIndex][cascadeIndex] = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            }
            else
            {
                m_cascadeShadowMaps[viewIndex][cascadeIndex].reset();
            }
        }
    }
}

void ShadowMapPass::resizeCascadeShadowMapIfNeeded(uint32_t size, uint32_t cascadeCount)
{
    size = size == 0 ? DEFAULT_SHADOW_MAP_SIZE : size;
    cascadeCount = std::clamp(cascadeCount, 1u, MAX_SHADOW_CASCADES);

    if (m_cascadeShadowMaps[m_currentShadowView][0] &&
        size == m_currentCascadeShadowMapSize &&
        cascadeCount == m_currentCascadeCount)
    {
        return;
    }

    createCascadeShadowMap(size, cascadeCount);
}

void ShadowMapPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};

    rootParameters[0].InitAsConstants(sizeof(ShadowDrawConstants) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[2].InitAsConstants(1, 2, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void ShadowMapPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"ShadowMapVertexShader.cso", &vertexShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

const LightComponent* ShadowMapPass::findMainShadowCastingDirectionalLight() const
{
    uint32_t directionalIndex = 0;
    for (const LightComponent* light : app->getModuleScene()->getLightComponents())
    {
        if (!light || !light->isActive()) continue;
        const GameObject* owner = light->getOwner();
        if (!owner || !owner->IsActiveInWindowHierarchy() || !owner->GetTransform()) continue;
        const LightData& data = light->getData();
        if (data.type != LightType::DIRECTIONAL) continue;
        if (directionalIndex++ >= LightDefaults::MAX_DIRECTIONAL_LIGHTS) break;
        if (data.shadow.castShadows) return light;
    }
    return nullptr;
}

void ShadowMapPass::prepareDisabledShadowData(const RenderContext& ctx)
{
    m_frameData = {};
    m_frameData.enabled = false;
    m_activeCascadeCount = 0;

    for (uint32_t i = 0; i < MAX_SHADOW_CASCADES; ++i)
    {
        const Texture* texture = m_cascadeShadowMaps[m_currentShadowView][i] ? 
            m_cascadeShadowMaps[m_currentShadowView][i].get() : 
            m_cascadeShadowMaps[m_currentShadowView][0].get();

        if (texture && texture->hasSRV()) 
        {
            m_frameData.cascadeShadowMapSRVs[i] = texture->getSRV().gpu;
        }
            
    }

    ShadowDataCB shadowCB{};
    shadowCB.shadowsEnabled = 0;
    shadowCB.shadowBias = SHADOW_BIAS;
    shadowCB.shadowStrength = SHADOW_STRENGTH;
    shadowCB.pcfEnabled = 0;
    shadowCB.pcfRadius = 1;

    if (ctx.ringBuffer != nullptr) m_frameData.shadowCBAddress = ctx.ringBuffer->allocate(&shadowCB, sizeof(ShadowDataCB));
}

void ShadowMapPass::prepareDirectionalShadowData(const RenderContext& ctx, const LightComponent& light)
{
    const LightShadowSettings& settings = light.getData().shadow;

    m_activeCascadeCount = std::clamp(settings.cascadeCount, 1u, MAX_SHADOW_CASCADES);

    resizeCascadeShadowMapIfNeeded(settings.shadowMapSize, m_activeCascadeCount);

    if (m_shadowFrustumComputePass == nullptr || !m_shadowFrustumComputePass->isEnabled() || !m_shadowFrustumComputePass->hasValidResult())
    {
        prepareDisabledShadowData(ctx);
        return;
    }

    const D3D12_GPU_VIRTUAL_ADDRESS shadowDataAddress = m_shadowFrustumComputePass->getShadowDataBufferAddress();

    if (shadowDataAddress == 0)
    {
        prepareDisabledShadowData(ctx);
        return;
    }

    m_frameData = {};
    m_frameData.enabled = true;
    m_frameData.shadowCBAddress = shadowDataAddress;

    for (uint32_t i = 0; i < MAX_SHADOW_CASCADES; ++i)
    {
        const uint32_t sourceIndex = i < m_activeCascadeCount ? i : 0;

        if (m_cascadeShadowMaps[m_currentShadowView][sourceIndex] && m_cascadeShadowMaps[m_currentShadowView][sourceIndex]->hasSRV())
        {
            m_frameData.cascadeShadowMapSRVs[i] = m_cascadeShadowMaps[m_currentShadowView][sourceIndex]->getSRV().gpu;
        }
    }
}

void ShadowMapPass::buildShadowCasterList(const RenderContext& ctx, const LightComponent& light)
{
    m_shadowCasters.clear();

    const GameObject* owner = light.getOwner();
    const Transform* transform = owner != nullptr ? owner->GetTransform() : nullptr;

    if (transform == nullptr) return;

    Vector3 lightDirection = transform->getForward();
    if (lightDirection.LengthSquared() <= 0.000001f) return;

    Matrix lightViewProjection;

    if (!buildConservativeLightViewProjection(ctx, lightDirection, 20.0f, lightViewProjection))
    {
        return;
    }

    for (MeshRenderer* renderer : app->getModuleScene()->getMeshRenderers())
    {
        if (renderer == nullptr || !renderer->isActive() || !renderer->hasMesh()) continue;

        GameObject* rendererOwner = renderer->getOwner();
        if (rendererOwner == nullptr || !rendererOwner->IsActiveInWindowHierarchy() || renderer->getTransform() == nullptr) continue;

        if (intersectsLightFrustum(*renderer, lightViewProjection)) m_shadowCasters.push_back(renderer);
    }
}

void ShadowMapPass::renderCasters(ID3D12GraphicsCommandList4* commandList, uint32_t cascadeIndex)
{
    (void)cascadeIndex;

    for (MeshRenderer* renderer : m_shadowCasters)
    {
        if (renderer != nullptr)
        {
            renderMeshRenderer(commandList, *renderer);
        }
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

    const VertexBuffer* gpuSkinnedVB = skin != nullptr ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* cpuSkinnedVB = skin != nullptr && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

    const bool useGpuSkinnedVB = gpuSkinnedVB != nullptr;
    const bool useCpuSkinnedVB = !useGpuSkinnedVB && cpuSkinnedVB != nullptr;
    const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

    const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : useCpuSkinnedVB ? cpuSkinnedVB : staticVB;

    if (activeVB == nullptr)
    {
        return;
    }

    const Matrix model = useWorldSpaceSkinnedVB ? Matrix::Identity : transform->getGlobalMatrix();

    ShadowDrawConstants constants{};
    constants.model = model.Transpose();

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(ShadowDrawConstants) / sizeof(UINT32), &constants, 0);

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
        commandList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.indexStart, 0, 0);
    }
}


void ShadowMapPass::transitionCascadeShadowMap(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState)
{
    if (!commandList) return;

    for (uint32_t i = 0; i < MAX_SHADOW_CASCADES; ++i)
    {
        if (!m_cascadeShadowMaps[m_currentShadowView][i] || m_cascadeShadowMapStates[m_currentShadowView][i] == newState) continue;

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_cascadeShadowMaps[m_currentShadowView][i]->getD3D12Resource().Get(),
            m_cascadeShadowMapStates[m_currentShadowView][i],
            newState);

        commandList->ResourceBarrier(1, &barrier);
        m_cascadeShadowMapStates[m_currentShadowView][i] = newState;
    }
}

void ShadowMapPass::transitionCascadeShadowMap(ID3D12GraphicsCommandList4* commandList, uint32_t cascadeIndex, D3D12_RESOURCE_STATES newState)
{
    if (!commandList || cascadeIndex >= MAX_SHADOW_CASCADES) return;

    auto& shadowMap = m_cascadeShadowMaps[m_currentShadowView][cascadeIndex];
    auto& state = m_cascadeShadowMapStates[m_currentShadowView][cascadeIndex];

    if (!shadowMap || state == newState) return;

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(shadowMap->getD3D12Resource().Get(), state, newState);
    commandList->ResourceBarrier(1, &barrier);
    state = newState;
}

void ShadowMapPass::prepare(const RenderContext& ctx)
{
    m_currentShadowView = ctx.viewType == RenderViewType::Game ? 1u : 0u;

    const LightComponent* light = findMainShadowCastingDirectionalLight();

    if (light == nullptr)
    {
        m_shadowCasters.clear();
        prepareDisabledShadowData(ctx);
        return;
    }

    buildShadowCasterList(ctx, *light);
    prepareDirectionalShadowData(ctx, *light);
}

void ShadowMapPass::debugDraw()
{
}

void ShadowMapPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (commandList == nullptr) return;

    BEGIN_EVENT(commandList, "ShadowMapPass");

    if (m_cascadeShadowMaps[m_currentShadowView][0] == nullptr || !m_cascadeShadowMaps[m_currentShadowView][0]->hasDSV())
    {
        END_EVENT(commandList);
        return;
    }

    if (!m_frameData.enabled)
    {
        transitionCascadeShadowMap(commandList, CASCADE_SHADER_RESOURCE_STATE);
        END_EVENT(commandList);
        return;
    }

    const D3D12_GPU_VIRTUAL_ADDRESS shadowDataAddress = m_frameData.shadowCBAddress;

    if (shadowDataAddress == 0)
    {
        transitionCascadeShadowMap(commandList, CASCADE_SHADER_RESOURCE_STATE);
        END_EVENT(commandList);
        return;
    }

    updateShadowViewportAndScissor(m_currentCascadeShadowMapSize);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(1, shadowDataAddress);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Cascaded shadow maps
    if (m_cascadeShadowMaps[m_currentShadowView][0] != nullptr &&
        m_cascadeShadowMaps[m_currentShadowView][0]->hasDSV() &&
        m_activeCascadeCount > 0)
    {
        const uint32_t updateMask = m_shadowFrustumComputePass != nullptr
            ? m_shadowFrustumComputePass->getCascadeUpdateMask()
            : ((1u << m_activeCascadeCount) - 1u);

        for (uint32_t cascadeIndex = 0; cascadeIndex < m_activeCascadeCount; ++cascadeIndex)
        {
            if ((updateMask & (1u << cascadeIndex)) == 0u) continue;

            Texture& cascade = *m_cascadeShadowMaps[m_currentShadowView][cascadeIndex];

            transitionCascadeShadowMap(commandList, cascadeIndex, D3D12_RESOURCE_STATE_DEPTH_WRITE);

            updateShadowViewportAndScissor(cascade.getDesc().width);
            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            const D3D12_CPU_DESCRIPTOR_HANDLE cascadeDSV = cascade.getDSV().cpu;

            commandList->OMSetRenderTargets(0, nullptr, false, &cascadeDSV);
            commandList->ClearDepthStencilView(cascadeDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            commandList->SetGraphicsRoot32BitConstant(2, cascadeIndex, 0);

            renderCasters(commandList, cascadeIndex);

            transitionCascadeShadowMap(commandList, cascadeIndex, CASCADE_SHADER_RESOURCE_STATE);
        }
    }
    END_EVENT(commandList);
}
