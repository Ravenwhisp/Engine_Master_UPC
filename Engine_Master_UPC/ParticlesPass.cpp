#include "Globals.h"
#include "ParticlesPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include <d3dcompiler.h>
#include "PlatformHelpers.h"
#include "ModuleDescriptors.h"
#include "ModuleRender.h"
#include "ModuleResources.h"
#include "ModuleScene.h"
#include "ModuleAssets.h"
#include "ParticleSystemComponent.h"
#include "ParticleCommands.h"

#include "VertexBuffer.h"
#include "Texture.h"
#include "StructuredBuffer.h"

ParticlesPass::ParticlesPass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    CD3DX12_ROOT_PARAMETER rootParameters[4] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE samplerRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);

    rootParameters[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

    m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

    ComPtr<ID3DBlob> vertexShaderBlob;
    D3DReadFileToBlob(L"UIVertexShader.cso", &vertexShaderBlob);

    ComPtr<ID3DBlob> pixelShaderBlob;
    D3DReadFileToBlob(L"UIPixelShader.cso", &pixelShaderBlob);

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE; // DEBUG
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc = { 1, 0 };

    m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

    const Vertex quadVertices[6] =
    {
        { {-0.5f, -0.5f}, {0,0} },
        { { 0.5f, -0.5f}, {1,0} },
        { { 0.5f,  0.5f}, {1,1} },

        { {-0.5f, -0.5f}, {0,0} },
        { { 0.5f,  0.5f}, {1,1} },
        { {-0.5f,  0.5f}, {0,1} }
    };

    m_quadVertexBuffer.reset(app->getModuleResources()->createVertexBuffer(quadVertices, 6, sizeof(Vertex)));

    m_particleBuffer = app->getModuleResources()->createStructuredBuffer(sizeof(shaderParticleData), 100000, nullptr);
}

void ParticlesPass::prepare(const RenderContext& ctx)
{
    m_viewport = &ctx.viewport;

	//Placeholder texture for testing
    auto textureAsset = app->getModuleAssets()->loadAtPath<TextureAsset>("Assets/Textures/checkboard.jpg");
    m_testTexture = app->getModuleResources()->createTextureSRGB(*textureAsset);
    Texture* passedText = m_testTexture.get();

    static std::vector<ParticleEmitterCommand> test =
    {
        {
            passedText,
            {
                {
                    Vector3(0.f, 0.f, 0.f),
                    Vector2(1.f, 1.f),
                    0.f,
                    Vector4(1.f, 1.f, 1.f, 1.f)
                },
                {
                    Vector3(0.f, 5.f, 0.f),
                    Vector2(2.f, 2.f),
                    0.f,
                    Vector4(1.f, 1.f, 1.f, 0.5f)
                }
            }
        }
    };

    m_commands = &test;

    m_view = &ctx.view;
    m_projection = &ctx.projection;
}

void ParticlesPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap()
    };

    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    commandList->SetGraphicsRootDescriptorTable(3, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VERTEX_BUFFER_VIEW vbv = m_quadVertexBuffer->getVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    renderImages(commandList);
}

void ParticlesPass::renderImages(ID3D12GraphicsCommandList4* commandList)
{
    Matrix vp = buildImageVP().Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &vp, 0);

    for (const auto& command : *m_commands)
    {
        if (!command.texture || command.particles.empty())
            continue;

        const auto srv = command.texture->getSRV();
        if (!srv.IsShaderVisible() || srv.gpu.ptr == 0)
            continue;

        std::vector<shaderParticleData> gpuData(command.particles.size());

        for (size_t i = 0; i < command.particles.size(); ++i)
        {
            gpuData[i].worldPosition = buildImageWorldMatrix(command.particles[i]).Transpose();
            gpuData[i].colorAndAlpha = command.particles[i].colorAndAlpha;
        }

        m_particleBuffer->update(commandList, gpuData.data(), (uint32_t)gpuData.size());

        commandList->SetGraphicsRootDescriptorTable(1, m_particleBuffer->getSRVGPU());
        commandList->SetGraphicsRootDescriptorTable(2, srv.gpu);

        commandList->DrawInstanced(6, (UINT)command.particles.size(), 0, 0);
    }
}

Matrix ParticlesPass::buildImageWorldMatrix(const ParticleCommand& command) const
{
    Matrix rot = *m_view;
    rot._41 = rot._42 = rot._43 = 0.0f;
    Matrix rotInv = rot.Transpose();

    Vector3 scale = Vector3(command.scale.x, command.scale.y, 1.0f);

    return Matrix::CreateScale(scale) * Matrix::CreateRotationZ(command.rotationZ) * rotInv * Matrix::CreateTranslation(command.position);
}

Matrix ParticlesPass::buildImageVP()
{
    return (*m_view) * (*m_projection);
}