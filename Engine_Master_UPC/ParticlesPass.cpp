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
#include "ModuleCamera.h" // to test createBillboard()
#include "ParticleSystemComponent.h"
#include "ParticleCommands.h"


#include "VertexBuffer.h"
#include "Texture.h"

ParticlesPass::ParticlesPass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    CD3DX12_ROOT_PARAMETER rootParameters[5] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE samplerRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);

    rootParameters[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b0 <- view, projection
    rootParameters[1].InitAsConstants(sizeof(Vector2) / sizeof(UINT32), 1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b1 <- emitter u, v scaling (for tile animation)
    
    rootParameters[2].InitAsShaderResourceView(1); // t1 <- particle data (could we join it with the srvRange?)
    rootParameters[3].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL); // t0
    rootParameters[4].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL); // s0

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(5, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"ParticleVertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"ParticlePixelShader.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      //
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // ALPHA BLEND! (needs the particle order; we should have ADDITIVE BLEND as an alternate option)
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; // or D3D12_BLEND_ONE?
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; // HDR scene target
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

    /*
    const Vertex quadVertices[6] =
    {
        { Vector2(-0.5f, -0.5f), Vector2(1.0f, 1.0f)},
        { Vector2(0.5f, -0.5f), Vector2(0.0f, 1.0f)},
        { Vector2(0.5f, 0.5f), Vector2(0.0f, 0.0f)},

        { Vector2(-0.5f, -0.5f), Vector2(1.0f, 1.0f)},
        { Vector2(0.5f, 0.5f), Vector2(0.0f, 0.0f)},
        { Vector2(-0.5f, 0.5f), Vector2(1.0f, 0.0f)}
    };
    */

    const Vertex quadVertices[6] =
    {
        { Vector2(0.5f, 0.5f), Vector2(1.0f, 0.0f)},
        { Vector2(0.5f, -0.5f), Vector2(1.0f, 1.0f)},
        { Vector2(-0.5f, -0.5f), Vector2(0.0f, 1.0f)},

        { Vector2(-0.5f, 0.5f), Vector2(0.0f, 0.0f)},
        { Vector2(0.5f, 0.5f), Vector2(1.0f, 0.0f)},
        { Vector2(-0.5f, -0.5f), Vector2(0.0f, 1.0f)}
    };
    

    /*
     // SpriteRender quad
    const Vertex quadVertices[6] =
    {
        { Vector2(-0.5f, -0.5f), Vector2(0.0f, 1.0f) },
        { Vector2(0.5f, -0.5f), Vector2(1.0f, 1.0f) },
        { Vector2(0.5f,  0.5f), Vector2(1.0f, 0.0f) },

        { Vector2(-0.5f, -0.5f), Vector2(0.0f, 1.0f) },
        { Vector2(0.5f,  0.5f), Vector2(1.0f, 0.0f) },
        { Vector2(-0.5f,  0.5f), Vector2(0.0f, 0.0f) }
    };
    */
    

    m_quadVertexBuffer.reset(app->getModuleResources()->createVertexBuffer(quadVertices, 6, sizeof(Vertex)));
}

void ParticlesPass::prepare(const RenderContext& ctx)
{
    m_viewport = &ctx.viewport;
    
    m_commands = ctx.particleCommands;
    
    m_view = &ctx.view;
    m_projection = &ctx.projection;
    m_cameraPosition = &ctx.cameraPosition;
}

void ParticlesPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    //if (PIXIsAttachedForGpuCapture()) PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);
    BEGIN_EVENT(commandList, "Particle rendering");

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootDescriptorTable(4, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VERTEX_BUFFER_VIEW vbv = m_quadVertexBuffer->getVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    renderImages(commandList);

    END_EVENT(commandList);
    //if (PIXIsAttachedForGpuCapture()) PIXEndCapture(TRUE);
}

void ParticlesPass::renderImages(ID3D12GraphicsCommandList4* commandList)
{
    Matrix vp = buildImageVP().Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &vp, 0);

    for (const auto& command : *m_commands)
    {
        if (!command.texture || command.particles.empty())
        {
            continue;
        }

        const auto srv = command.texture->getSRV();
        if (!srv.IsShaderVisible() || srv.gpu.ptr == 0)
        {
            continue;
        }
        const size_t size = command.particles.size();
        shaderParticleData* particleData = new shaderParticleData[size];

        const std::vector<ParticleSystemComponent*>& particleSystemComponents = app->getModuleScene()->getParticleSystemComponents();
        for (unsigned int i = 0; i < size; ++i)
        {
            XMMATRIX m = buildImageWorldMatrix(command.particles[i], command.renderMode).Transpose();
            XMStoreFloat4x4(&particleData[i].worldPosition, m);

            particleData[i].colorAndAlpha = command.particles[i].colorAndAlpha;
            particleData[i].sheetOffset = command.particles[i].sheetOffset;
        }

        commandList->SetGraphicsRootShaderResourceView(
            2,
            app->getModuleRender()->allocateInRingBuffer(particleData, size * sizeof(shaderParticleData) )
        );

        delete[] particleData;

        commandList->SetGraphicsRootDescriptorTable(3, srv.gpu);
        commandList->SetGraphicsRoot32BitConstants(1, sizeof(XMFLOAT2) / sizeof(UINT32), &command.uvScale, 0);

        commandList->DrawInstanced(6, command.particles.size(), 0, 0); // last is  first instanceID to consider; here we will take all of them, so 0
    }
}


Matrix ParticlesPass::buildImageWorldMatrix(const ParticleCommand& command, EmitterRender::RenderMode mode) const
{
    Matrix view = *m_view;

    Vector3 scale = Vector3(command.scale.x, command.scale.y, 1.0f);
    Matrix scaleMat = Matrix::CreateScale(scale);
    Matrix rotZMat = Matrix::CreateRotationZ(command.rotationZ);
    Matrix transMat = Matrix::CreateTranslation(command.position);

    switch (mode)
    {
    case EmitterRender::RenderMode::BILLBOARD:
    {
        Matrix rot = view;
        rot._41 = rot._42 = rot._43 = 0.0f;
        Matrix rotInv = rot.Transpose();

        return scaleMat * rotZMat * rotInv * transMat;
    }

    case EmitterRender::RenderMode::HORIZONTAL:
    {
        // Rotamos el quad 90 grados en X para alinearlo con el suelo
        Matrix rotX = Matrix::CreateRotationX(-1.57079633f);

        return scaleMat * rotZMat * rotX * transMat;
    }

    case EmitterRender::RenderMode::VERTICAL:
    {
        // Extraer la posición de la cámara
        Matrix invView = view.Invert();
        Vector3 camPos = Vector3(invView._41, invView._42, invView._43);

        // Dirección hacia la cámara (ignorando la altura para que sea cilíndrico)
        Vector3 lookAt = command.position - camPos;
        lookAt.y = 0.0f;

        if (lookAt.LengthSquared() < 0.001f) {
            lookAt = Vector3::Forward;
        }
        else {
            lookAt.Normalize();
        }

        Matrix billboardMat = Matrix::CreateLookAt(Vector3::Zero, lookAt, Vector3::Up);
        Matrix rotInv = billboardMat.Transpose();

        return scaleMat * rotZMat * rotInv * transMat;
    }
    }

    return scaleMat * rotZMat * transMat;
}

Matrix ParticlesPass::buildImageVP()
{
    return (*m_view) * (*m_projection);
}
