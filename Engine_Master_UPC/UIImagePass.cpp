#include "Globals.h"
#include "UIImagePass.h"
#include "UIParams.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include <d3dcompiler.h>
#include "PlatformHelpers.h"
#include "ModuleDescriptors.h"
#include "ModuleRender.h"
#include "ModuleResources.h"

#include "VertexBuffer.h"
#include "Texture.h"
#include <algorithm>

UIImagePass::UIImagePass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE samplerRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);

    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b0
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL); // t0
    rootParameters[2].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL); // s0

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"UIVertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"UIPixelShader.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateDepth)));

    const UIVertex quadVertices[6] =
    {
        { Vector2(0.0f, 0.0f), Vector2(0.0f, 1.0f), Vector4(1,1,1,1) },
        { Vector2(1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector4(1,1,1,1) },
        { Vector2(1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector4(1,1,1,1) },

        { Vector2(0.0f, 0.0f), Vector2(0.0f, 1.0f), Vector4(1,1,1,1) },
        { Vector2(1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector4(1,1,1,1) },
        { Vector2(0.0f, 1.0f), Vector2(0.0f, 0.0f), Vector4(1,1,1,1) }
    };

    m_quadVertexBuffer.reset(app->getModuleResources()->createVertexBuffer(quadVertices, 6, sizeof(UIVertex)));
}

void UIImagePass::prepare(const RenderContext& ctx)
{
    m_viewport = &ctx.viewport;
    m_commands = ctx.uiImageCommands;
    m_view = &ctx.view;
    m_projection = &ctx.projection;

    m_sortedCommands = *m_commands;
    std::stable_sort(m_sortedCommands.begin(), m_sortedCommands.end(), compareUI);
}

void UIImagePass::apply(ID3D12GraphicsCommandList4* commandList)
{
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = {app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootDescriptorTable( 2, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VERTEX_BUFFER_VIEW vbv = m_quadVertexBuffer->getVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    renderImages(commandList);
}

void UIImagePass::renderImages(ID3D12GraphicsCommandList4* commandList)
{
    for (const auto& command : m_sortedCommands)
    {
        if (!command.texture)
        {
            continue;
        }

        const auto srv = command.texture->getSRV();
        if (!srv.IsShaderVisible() || srv.gpu.ptr == 0)
        {
            continue;
        }

        if (command.renderMode == CanvasRenderMode::SCREEN_SPACE)
        {
            commandList->SetPipelineState(m_pipelineState.Get());
        }
        else
        {
            commandList->SetPipelineState(m_pipelineStateDepth.Get());
        }

        UIParams params{};
        params.mvp = buildImageMVP(command).Transpose();

        const float aspectRatio = (command.rect.h > 0.0f)
            ? (command.rect.w / command.rect.h)
            : 1.0f;

        params.fillData = Vector4(
            command.fillAmount,
            static_cast<float>(command.fillMethod),
            static_cast<float>(command.fillOrigin),
            aspectRatio);

        params.alpha = command.alpha;
        params.sheetOffset = Vector2(command.sheetOffset.x, command.sheetOffset.y);
        params.uvScale = Vector2(command.uvScale.x, command.uvScale.y);
        if (command.renderMode == CanvasRenderMode::SCREEN_SPACE)
        {
			params.flipY = 1.0f;
        }

        commandList->SetGraphicsRootConstantBufferView(
            0,
            app->getModuleRender()->allocateInRingBuffer(&params, sizeof(UIParams))
        );

        commandList->SetGraphicsRootDescriptorTable(1, srv.gpu);

        commandList->DrawInstanced(6, 1, 0, 0);
    }
}

Matrix UIImagePass::buildImageMVP(const UIImageCommand& command) const
{
    const float x = command.rect.x;
    const float y = command.rect.y;
    const float w = command.rect.w;
    const float h = command.rect.h;

    Matrix local;

    if (command.renderMode == CanvasRenderMode::SCREEN_SPACE)
    {
		Matrix scale = Matrix::CreateScale(w, h, 1.0f);
		Matrix translate = Matrix::CreateTranslation(x, y, 0.0f);
        local = scale * translate;

        const float viewportWidth = m_viewport->Width;
        const float viewportHeight = m_viewport->Height;
		Matrix ortho = Matrix::CreateOrthographicOffCenter(0.0f, viewportWidth, viewportHeight, 0.0f, 0.0f, 1.0f);

		return local * ortho;
    }

    const float uiToWorld = 0.01f;

    Matrix scale = Matrix::CreateScale(w * uiToWorld, h * uiToWorld, uiToWorld);
	Matrix translate = Matrix::CreateTranslation(x * uiToWorld, y * uiToWorld, 0.0f);

    local = scale * translate;

	Matrix world = command.world;

    if (command.renderMode == CanvasRenderMode::WORLD_SPACE_CAMERA)
    {
        Matrix invView = m_view->Invert();
        invView._11 *= -1.0f;
        invView._12 *= -1.0f;
        invView._13 *= -1.0f;
        invView._41 = world._41;
        invView._42 = world._42;
        invView._43 = world._43;
		world = invView;
    }

    return (local * world) * (*m_view) * (*m_projection);
}

bool UIImagePass::compareUI(const UIImageCommand& a, const UIImageCommand& b)
{
    auto ak = std::make_tuple(
        a.renderMode != CanvasRenderMode::SCREEN_SPACE,
        a.zTest
    );

    auto bk = std::make_tuple(
        b.renderMode != CanvasRenderMode::SCREEN_SPACE,
        b.zTest
    );

    return ak > bk;
}