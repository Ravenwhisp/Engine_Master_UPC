#include "Globals.h"
#include "DepthReductionPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"

#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include "PlatformHelpers.h"

DepthReductionPass::DepthReductionPass(
    ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    createRootSignature();
    createPipelineStates();
}

uint32_t DepthReductionPass::divideRoundUp(
    uint32_t value,
    uint32_t divisor)
{
    return (value + divisor - 1u) / divisor;
}

void DepthReductionPass::createRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    srvRange.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        1,
        0,
        0);

    CD3DX12_DESCRIPTOR_RANGE uavRange;
    uavRange.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
        1,
        0,
        0);

    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};

    rootParameters[0].InitAsDescriptorTable(
        1,
        &srvRange,
        D3D12_SHADER_VISIBILITY_ALL);

    rootParameters[1].InitAsDescriptorTable(
        1,
        &uavRange,
        D3D12_SHADER_VISIBILITY_ALL);

    rootParameters[2].InitAsConstants(
        sizeof(ReductionConstants) / sizeof(uint32_t),
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

void DepthReductionPass::createPipelineStates()
{
    ComPtr<ID3DBlob> initialShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(
        L"DepthMinMaxInitialComputeShader.cso",
        &initialShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC initialPSODesc{};
    initialPSODesc.pRootSignature = m_rootSignature.Get();
    initialPSODesc.CS =
        CD3DX12_SHADER_BYTECODE(initialShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(
        &initialPSODesc,
        IID_PPV_ARGS(&m_initialReductionPipelineState)));

    ComPtr<ID3DBlob> reductionShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(
        L"DepthMinMaxReduceComputeShader.cso",
        &reductionShaderBlob));

    D3D12_COMPUTE_PIPELINE_STATE_DESC reductionPSODesc{};
    reductionPSODesc.pRootSignature = m_rootSignature.Get();
    reductionPSODesc.CS =
        CD3DX12_SHADER_BYTECODE(reductionShaderBlob.Get());

    DXCall(m_device->CreateComputePipelineState(
        &reductionPSODesc,
        IID_PPV_ARGS(&m_reductionPipelineState)));
}

void DepthReductionPass::ensureReductionTextures(
    uint32_t depthWidth,
    uint32_t depthHeight)
{
    const uint32_t requiredWidth =
        divideRoundUp(depthWidth, TILE_SIZE);

    const uint32_t requiredHeight =
        divideRoundUp(depthHeight, TILE_SIZE);

    if (m_pingTexture != nullptr &&
        m_pongTexture != nullptr &&
        requiredWidth == m_reductionTextureWidth &&
        requiredHeight == m_reductionTextureHeight)
    {
        return;
    }

    m_pingTexture.reset(
        app->getModuleResources()->createDepthMinMaxTexture(
            requiredWidth,
            requiredHeight));

    m_pongTexture.reset(
        app->getModuleResources()->createDepthMinMaxTexture(
            requiredWidth,
            requiredHeight));

    if (m_pingTexture != nullptr)
    {
        m_pingTexture->setName(
            L"DepthMinMaxReduction_Ping");
    }

    if (m_pongTexture != nullptr)
    {
        m_pongTexture->setName(
            L"DepthMinMaxReduction_Pong");
    }

    m_reductionTextureWidth = requiredWidth;
    m_reductionTextureHeight = requiredHeight;

    m_resultTexture = nullptr;
}

void DepthReductionPass::prepare(const RenderContext& ctx)
{
    m_inputDepthTexture =
        ctx.renderSurface
        .getTexture(RenderSurface::DEPTH_STENCIL)
        .get();

    m_resultTexture = nullptr;

    if (m_inputDepthTexture == nullptr)
    {
        m_depthWidth = 0;
        m_depthHeight = 0;
        return;
    }

    const TextureDesc depthDesc =
        m_inputDepthTexture->getDesc();

    m_depthWidth = depthDesc.width;
    m_depthHeight = depthDesc.height;

    if (m_depthWidth == 0 || m_depthHeight == 0)
    {
        return;
    }

    ensureReductionTextures(
        m_depthWidth,
        m_depthHeight);
}

void DepthReductionPass::transitionTexture(
    ID3D12GraphicsCommandList4* commandList,
    Texture& texture,
    D3D12_RESOURCE_STATES beforeState,
    D3D12_RESOURCE_STATES afterState)
{
    if (beforeState == afterState)
    {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            texture.getD3D12Resource().Get(),
            beforeState,
            afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void DepthReductionPass::dispatchReductionStage(
    ID3D12GraphicsCommandList4* commandList,
    ID3D12PipelineState* pipelineState,
    Texture& inputTexture,
    Texture& outputTexture,
    uint32_t inputWidth,
    uint32_t inputHeight)
{
    const uint32_t outputWidth =
        divideRoundUp(inputWidth, TILE_SIZE);

    const uint32_t outputHeight =
        divideRoundUp(inputHeight, TILE_SIZE);

    transitionTexture(
        commandList,
        outputTexture,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->SetPipelineState(pipelineState);

    commandList->SetComputeRootDescriptorTable(
        0,
        inputTexture.getSRV().gpu);

    commandList->SetComputeRootDescriptorTable(
        1,
        outputTexture.getUAV().gpu);

    ReductionConstants constants{};
    constants.inputWidth = inputWidth;
    constants.inputHeight = inputHeight;

    commandList->SetComputeRoot32BitConstants(
        2,
        sizeof(ReductionConstants) / sizeof(uint32_t),
        &constants,
        0);

    commandList->Dispatch(
        outputWidth,
        outputHeight,
        1);

    CD3DX12_RESOURCE_BARRIER uavBarrier =
        CD3DX12_RESOURCE_BARRIER::UAV(
            outputTexture.getD3D12Resource().Get());

    commandList->ResourceBarrier(
        1,
        &uavBarrier);

    transitionTexture(
        commandList,
        outputTexture,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void DepthReductionPass::apply(
    ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "DepthReductionPass");

    if (commandList == nullptr ||
        m_inputDepthTexture == nullptr ||
        m_pingTexture == nullptr ||
        m_pongTexture == nullptr ||
        m_depthWidth == 0 ||
        m_depthHeight == 0)
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

    commandList->SetComputeRootSignature(
        m_rootSignature.Get());

    transitionTexture(
        commandList,
        *m_inputDepthTexture,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    dispatchReductionStage(
        commandList,
        m_initialReductionPipelineState.Get(),
        *m_inputDepthTexture,
        *m_pingTexture,
        m_depthWidth,
        m_depthHeight);

    Texture* currentInput = m_pingTexture.get();
    Texture* currentOutput = m_pongTexture.get();

    uint32_t currentWidth =
        divideRoundUp(m_depthWidth, TILE_SIZE);

    uint32_t currentHeight =
        divideRoundUp(m_depthHeight, TILE_SIZE);

    while (currentWidth > 1 || currentHeight > 1)
    {
        dispatchReductionStage(
            commandList,
            m_reductionPipelineState.Get(),
            *currentInput,
            *currentOutput,
            currentWidth,
            currentHeight);

        currentWidth =
            divideRoundUp(currentWidth, TILE_SIZE);

        currentHeight =
            divideRoundUp(currentHeight, TILE_SIZE);

        Texture* previousInput = currentInput;
        currentInput = currentOutput;
        currentOutput = previousInput;
    }

    m_resultTexture = currentInput;

    transitionTexture(
        commandList,
        *m_inputDepthTexture,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);

    END_EVENT(commandList);
}
