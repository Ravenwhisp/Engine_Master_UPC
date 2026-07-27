#include "Globals.h"
#include "TrailPass.h"
#include "BasicMaterial.h"
#include "ModuleDescriptors.h"
#include "Application.h"
#include "ModuleScene.h"
#include "RenderContext.h"
#include "TrailComponent.h"
#include "GameObject.h"
#include "Transform.h"
#include "RingBuffer.h"
#include "ModuleRender.h"

#include <d3dcompiler.h>
#include "PlatformHelpers.h"

TrailPass::TrailPass(ComPtr<ID3D12Device4> device)
{
    m_device = device;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange, sampRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParameters[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // b0 <- view, projection

    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));


#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    // Load the vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"TrailVertexShader.cso", &vertexShaderBlob));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"TrailPixelShader.cso", &pixelShaderBlob));

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      
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

}

void TrailPass::prepare(const RenderContext& ctx)
{
    if (m_ringBuffer == nullptr) m_ringBuffer = ctx.ringBuffer;
    
    m_viewport = &ctx.viewport;

    m_view = &ctx.view;
    m_projection = &ctx.projection;
    m_cameraPosition = &ctx.cameraPosition;

    m_trailComponent = app->getModuleScene()->getTrailComponents();
}

void TrailPass::apply(ID3D12GraphicsCommandList4* commandList)
{

    std::vector<UINT> indices;
    std::vector<VertexTrails> vertices;

    uint32_t firstVertex = 0;

    for (auto& trailComponent : m_trailComponent) 
    {

        GameObject* owner = trailComponent->getOwner();
        if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
        {
            continue;
        }

        if (!trailComponent->isActive())
        {
            continue;
        }

        if (trailComponent->getTrailPoints().size() <= 1)
        {
            continue;
        }


        for (auto point = trailComponent->getTrailPoints().begin(); point != trailComponent->getTrailPoints().end(); point++)
        {
            Vector3 position = point->get()->position;
            Vector3 perpendicularVector = Vector3::Transform(Vector3::UnitX, point->get()->rotation);
            float halfWidth = point->get()->width * 0.5f;

            Vector3 prevPos = (point == trailComponent->getTrailPoints().begin()) ? point->get()->position : std::prev(point)->get()->position;

            Vector3 nextPos = (point == std::prev(trailComponent->getTrailPoints().end())) ? owner->GetTransform()->getGlobalMatrix().Translation() : std::next(point)->get()->position;

            Vector3 tangent = nextPos - prevPos;
            Vector3 right = tangent;
            right.Cross(Vector3::Up).Normalize();
            Vector3 normal = right;
            normal.Cross(tangent).Normalize();


            VertexTrails leftVertex{};
            leftVertex.position = position - perpendicularVector * halfWidth;
            leftVertex.tangent = tangent;
            leftVertex.normal = normal;
            leftVertex.texCoord0 = Vector2::Zero;
            leftVertex.color = point->get()->color;

            VertexTrails rightVertex{};
            rightVertex.position = position + perpendicularVector * halfWidth;
            rightVertex.tangent = tangent;
            rightVertex.normal = normal;
            rightVertex.texCoord0 = Vector2::Zero;
            rightVertex.color = point->get()->color;

            vertices.push_back(leftVertex);
            vertices.push_back(rightVertex);
        }

        for (uint32_t i = firstVertex; i < vertices.size() - 2; i += 2)
        {

            indices.push_back(i);
            indices.push_back(i + 3);
            indices.push_back(i + 1);

            indices.push_back(i);
            indices.push_back(i + 2);
            indices.push_back(i + 3);
        }

        firstVertex = vertices.size();

    }

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = app->getModuleRender()->allocateInRingBuffer(vertices.data(), vertices.size() * sizeof(VertexTrails));
    vbv.SizeInBytes = (UINT)(vertices.size() * sizeof(VertexTrails));
    vbv.StrideInBytes = sizeof(VertexTrails);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = app->getModuleRender()->allocateInRingBuffer(indices.data(), indices.size() * sizeof(uint32_t));
    ibv.SizeInBytes = (UINT)(indices.size() * sizeof(uint32_t));
    ibv.Format = DXGI_FORMAT_R32_UINT;

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->IASetVertexBuffers(0, 1, &vbv);
    commandList->IASetIndexBuffer(&ibv);

    commandList->DrawIndexedInstanced((UINT)indices.size(), 1, 0, 0, 0);

    Matrix vp = (*m_view) * (*m_projection);
    commandList->SetGraphicsRoot32BitConstants(2, sizeof(XMMATRIX) / sizeof(UINT32), &vp, 0);
    
}
