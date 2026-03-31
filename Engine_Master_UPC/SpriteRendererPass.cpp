#include "Globals.h"
#include "SpriteRendererPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleRender.h"
#include "ModuleResources.h"
#include "ModuleScene.h"

#include "GameObject.h"
#include "Transform.h"
#include "SpriteRenderer.h"
#include "VertexBuffer.h"
#include "Texture.h"
#include "TextureAsset.h"

#include "SimpleMath.h"
#include <d3dcompiler.h>
#include "PlatformHelpers.h"

#include "SpriteVertex.h"
#include "SpriteParams.h"

SpriteRendererPass::SpriteRendererPass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE samplerRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);

    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"SpriteVertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"UIPixelShader.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

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
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

    const SpriteVertex quadVertices[6] =
    {
        { Vector3(-0.5f, -0.5f, 0.0f), Vector2(0.0f, 1.0f) },
        { Vector3(0.5f, -0.5f, 0.0f), Vector2(1.0f, 1.0f) },
        { Vector3(0.5f,  0.5f, 0.0f), Vector2(1.0f, 0.0f) },

        { Vector3(-0.5f, -0.5f, 0.0f), Vector2(0.0f, 1.0f) },
        { Vector3(0.5f,  0.5f, 0.0f), Vector2(1.0f, 0.0f) },
        { Vector3(-0.5f,  0.5f, 0.0f), Vector2(0.0f, 0.0f) }
    };

    m_quadVertexBuffer.reset(app->getModuleResources()->createVertexBuffer(quadVertices, 6, sizeof(SpriteVertex)));
}

void SpriteRendererPass::prepare(const RenderContext& ctx)
{
    m_view = &ctx.view;
    m_projection = &ctx.projection;

    m_spriteRenderers = app->getModuleScene()->getSpriteRenderers();
}

void SpriteRendererPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootDescriptorTable(2, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VERTEX_BUFFER_VIEW vbv = m_quadVertexBuffer->getVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    renderSprites(commandList);
}

void SpriteRendererPass::renderSprites(ID3D12GraphicsCommandList4* commandList)
{
    for (SpriteRenderer* sprite : m_spriteRenderers)
    {
        if (!sprite || !sprite->isActive())
        {
            continue;
        }

        GameObject* owner = sprite->getOwner();
        if (!owner || !owner->IsActiveInWindowHierarchy())
        {
            continue;
        }

        if (sprite->consumeLoadRequest())
        {
            TextureAsset* asset = sprite->getTextureAsset();

            if (!asset || sprite->getTextureAssetId() == INVALID_ASSET_ID)
            {
                sprite->setGpuTexture(nullptr);
            }
            else
            {
                auto texture = app->getModuleResources()->createTextureSRGB(*asset);
                sprite->setGpuTexture(texture);
            }
        }

        Texture* texture = sprite->getTexture();
        if (!texture)
        {
            continue;
        }

        const auto srv = texture->getSRV();
        if (!srv.IsShaderVisible() || srv.gpu.ptr == 0)
        {
            continue;
        }

        SpriteParams params{};
        params.mvp = buildSpriteMVP(sprite).Transpose();

        commandList->SetGraphicsRootConstantBufferView(0, app->getModuleRender()->allocateInRingBuffer(&params, sizeof(SpriteParams)));

        commandList->SetGraphicsRootDescriptorTable(1, srv.gpu);

        commandList->DrawInstanced(6, 1, 0, 0);
    }
}

Matrix SpriteRendererPass::buildSpriteMVP(SpriteRenderer* sprite) const
{
    GameObject* owner = sprite->getOwner();
    Transform* transform = owner->GetTransform();

    return transform->getGlobalMatrix() * (*m_view) * (*m_projection);
}