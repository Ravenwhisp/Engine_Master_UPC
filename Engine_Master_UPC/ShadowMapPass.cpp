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

#include "ShadowFrustumComputePass.h"
#include "ShadowCasterCullingPass.h"
#include "ShadowCasterTypes.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include "PlatformHelpers.h"

#include <algorithm>

namespace
{
    constexpr D3D12_RESOURCE_STATES CASCADE_SHADER_RESOURCE_STATE =
        static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

}

ShadowMapPass::ShadowMapPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass, ShadowCasterCullingPass* shadowCasterCullingPass)
    : m_device(device)
    , m_shadowFrustumComputePass(shadowFrustumComputePass)
    , m_shadowCasterCullingPass(shadowCasterCullingPass)
{
    createCascadeShadowMap(1u, 1u);
    createRootSignature();
    createCommandSignature();
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

void ShadowMapPass::createCommandSignature()
{
    D3D12_INDIRECT_ARGUMENT_DESC arguments[4]{};

    arguments[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
    arguments[0].Constant.RootParameterIndex = 0;
    arguments[0].Constant.DestOffsetIn32BitValues = 0;
    arguments[0].Constant.Num32BitValuesToSet = sizeof(ShadowDrawConstants) / sizeof(uint32_t);

    arguments[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
    arguments[1].VertexBuffer.Slot = 0;

    arguments[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;

    arguments[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC desc{};
    desc.ByteStride = sizeof(ShadowIndirectCommandGPU);
    desc.NumArgumentDescs = _countof(arguments);
    desc.pArgumentDescs = arguments;

    DXCall(m_device->CreateCommandSignature(&desc, m_rootSignature.Get(), IID_PPV_ARGS(&m_commandSignature)));
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

void ShadowMapPass::renderCastersIndirect(ID3D12GraphicsCommandList4* commandList, uint32_t cascadeIndex)
{
    if (commandList == nullptr || m_shadowCasterCullingPass == nullptr || m_commandSignature == nullptr) return;
    if (cascadeIndex >= MAX_SHADOW_CASCADES) return;

    ID3D12Resource* argumentBuffer = m_shadowCasterCullingPass->getIndirectCommandBuffer(cascadeIndex);
    ID3D12Resource* countBuffer = m_shadowCasterCullingPass->getCascadeCountBuffer();
    const uint32_t maxCommandCount = m_shadowCasterCullingPass->getCandidateCount();

    if (argumentBuffer == nullptr || countBuffer == nullptr || maxCommandCount == 0) return;

    const uint64_t countBufferOffset = static_cast<uint64_t>(cascadeIndex) * sizeof(uint32_t);

    commandList->ExecuteIndirect(m_commandSignature.Get(), maxCommandCount, argumentBuffer, 0, countBuffer, countBufferOffset);
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
        prepareDisabledShadowData(ctx);
        return;
    }

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

            renderCastersIndirect(commandList, cascadeIndex);

            transitionCascadeShadowMap(commandList, cascadeIndex, CASCADE_SHADER_RESOURCE_STATE);
        }
    }
    END_EVENT(commandList);
}
