#include "Globals.h"
#include "LightCullingPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "ModuleScene.h"

#include "DeferredShadingPass.h"
#include "RenderContext.h"
#include "RenderSurface.h"
#include "RingBuffer.h"
#include "Texture.h"
#include "Lights.h"

#include <algorithm>
#include <cstdint>
#include <d3dcompiler.h>
#include <d3dx12.h>

#include "PlatformHelpers.h"
#include "OptickProfiler.h"

LightCullingPass::LightCullingPass(ComPtr<ID3D12Device4> device, DeferredShadingPass* deferredShadingPass)
    : m_device(device)
    , m_deferredShadingPass(deferredShadingPass)
{
    createRootSignature();
    createPipelineState();
}

uint32_t LightCullingPass::divideRoundUp(uint32_t value, uint32_t divisor)
{
    return (value + divisor - 1u) / divisor;
}

void LightCullingPass::createRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE depthRange;
    depthRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[5] = {};

    rootParameters[0].InitAsDescriptorTable(1, &depthRange, D3D12_SHADER_VISIBILITY_ALL); // t0: depth texture
    rootParameters[1].InitAsUnorderedAccessView(0, 0);                                    // u0: point light index list
    rootParameters[2].InitAsUnorderedAccessView(1, 0);                                    // u1: spot light index list
    rootParameters[3].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);        // b2: LightsCB (shared layout with DeferredShadingPass)
    rootParameters[4].InitAsConstants(sizeof(TileCullingConstants) / sizeof(uint32_t), 0, 0, D3D12_SHADER_VISIBILITY_ALL); // b0

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void LightCullingPass::createPipelineState()
{
    ComPtr<ID3DBlob> computeShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"LightCullingComputeShader.cso", &computeShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void LightCullingPass::ensureTileListBuffers(uint32_t requiredTileCount)
{
    requiredTileCount = std::max(1u, requiredTileCount);

    if (m_pointLightIndexBuffer != nullptr &&
        m_spotLightIndexBuffer != nullptr &&
        requiredTileCount <= m_allocatedTileCapacity)
    {
        return;
    }

    // flush before growing - another viewport's command list might still reference the old buffer
    app->getModuleD3D12()->getCommandQueue()->flush();

    const size_t bufferSize = static_cast<size_t>(requiredTileCount) * MAX_LIGHTS_PER_TILE * sizeof(int32_t);

    m_pointLightIndexBuffer = app->getModuleResources()->createDefaultBuffer(
        bufferSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        "LightCullingPointIndices");

    m_spotLightIndexBuffer = app->getModuleResources()->createDefaultBuffer(
        bufferSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        "LightCullingSpotIndices");

    m_bufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    m_allocatedTileCapacity = requiredTileCount;
}

void LightCullingPass::prepare(const RenderContext& ctx)
{
    PERF_RENDER("LightCullingPass::prepare");

    m_hasValidInput = false;

    if (m_deferredShadingPass == nullptr)
    {
        return;
    }

    m_inputDepthTexture = ctx.renderSurface.getTexture(RenderSurface::DEPTH_STENCIL).get();

    if (m_inputDepthTexture == nullptr)
    {
        return;
    }

    const TextureDesc depthDesc = m_inputDepthTexture->getDesc();

    if (depthDesc.width == 0 || depthDesc.height == 0)
    {
        return;
    }

    const uint32_t tileCountX = divideRoundUp(depthDesc.width, TILE_SIZE);
    const uint32_t tileCountY = divideRoundUp(depthDesc.height, TILE_SIZE);

    ensureTileListBuffers(tileCountX * tileCountY);

    // own copy - we run before DeferredShadingPass::prepare(), so its address isn't valid yet
    GPULightsConstantBuffer lightsCB = m_deferredShadingPass->packLightsForGPU(
        app->getModuleScene()->getLightComponents(),
        LightDefaults::DEFAULT_AMBIENT_COLOR,
        LightDefaults::DEFAULT_AMBIENT_INTENSITY);

    m_lightsCBAddress = ctx.ringBuffer->allocate(
        &lightsCB,
        sizeof(GPULightsConstantBuffer),
        app->getModuleD3D12()->getCurrentFrame());

    m_constants.view = ctx.view.Transpose();
    m_constants.xScale = ctx.projection._11;
    m_constants.yScale = ctx.projection._22;
    m_constants.proj33 = ctx.projection._33;
    m_constants.proj43 = ctx.projection._43;
    m_constants.tileCountX = tileCountX;
    m_constants.tileCountY = tileCountY;
    m_constants.screenWidth = depthDesc.width;
    m_constants.screenHeight = depthDesc.height;

    m_hasValidInput = true;
}

void LightCullingPass::transitionBuffers(ID3D12GraphicsCommandList4* commandList, D3D12_RESOURCE_STATES newState)
{
    if (m_bufferState == newState)
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(m_pointLightIndexBuffer.Get(), m_bufferState, newState),
        CD3DX12_RESOURCE_BARRIER::Transition(m_spotLightIndexBuffer.Get(), m_bufferState, newState)
    };

    commandList->ResourceBarrier(_countof(barriers), barriers);

    m_bufferState = newState;
}

void LightCullingPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "LightCullingPass");

    if (commandList == nullptr ||
        !m_hasValidInput ||
        m_inputDepthTexture == nullptr ||
        m_pointLightIndexBuffer == nullptr ||
        m_spotLightIndexBuffer == nullptr)
    {
        END_EVENT(commandList);
        return;
    }

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetComputeRootSignature(m_rootSignature.Get());

    CD3DX12_RESOURCE_BARRIER depthToReadable = CD3DX12_RESOURCE_BARRIER::Transition(
        m_inputDepthTexture->getD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList->ResourceBarrier(1, &depthToReadable);

    transitionBuffers(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->SetComputeRootDescriptorTable(0, m_inputDepthTexture->getSRV().gpu);
    commandList->SetComputeRootUnorderedAccessView(1, m_pointLightIndexBuffer->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(2, m_spotLightIndexBuffer->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(3, m_lightsCBAddress);
    commandList->SetComputeRoot32BitConstants(4, sizeof(TileCullingConstants) / sizeof(uint32_t), &m_constants, 0);

    commandList->Dispatch(m_constants.tileCountX, m_constants.tileCountY, 1);

    CD3DX12_RESOURCE_BARRIER uavBarriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::UAV(m_pointLightIndexBuffer.Get()),
        CD3DX12_RESOURCE_BARRIER::UAV(m_spotLightIndexBuffer.Get())
    };

    commandList->ResourceBarrier(_countof(uavBarriers), uavBarriers);

    transitionBuffers(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    CD3DX12_RESOURCE_BARRIER depthBack = CD3DX12_RESOURCE_BARRIER::Transition(
        m_inputDepthTexture->getD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);

    commandList->ResourceBarrier(1, &depthBack);

    END_EVENT(commandList);
}

D3D12_GPU_VIRTUAL_ADDRESS LightCullingPass::getPointLightIndexBufferAddress() const
{
    return m_pointLightIndexBuffer != nullptr ? m_pointLightIndexBuffer->GetGPUVirtualAddress() : 0;
}

D3D12_GPU_VIRTUAL_ADDRESS LightCullingPass::getSpotLightIndexBufferAddress() const
{
    return m_spotLightIndexBuffer != nullptr ? m_spotLightIndexBuffer->GetGPUVirtualAddress() : 0;
}
