#include "Globals.h"
#include "OutlinePass.h"

#include "ModuleDescriptors.h"
#include "RenderContext.h"
#include "SceneDataCB.h"
#include "Texture.h"
#include "Application.h"
#include "RingBuffer.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"

#include "PlatformHelpers.h"
#include "OptickProfiler.h"

#include <d3dcompiler.h>

OutlinePass::OutlinePass(ComPtr<ID3D12Device4> device)
{
    m_device = device;
    m_sceneDataCB = std::make_unique<SceneDataCB>();

    createRootSignature();
    createPipelineState();
}

void OutlinePass::prepare(const RenderContext& ctx)
{
    PERF_RENDER("OutlinePass::prepare");

    {
        PERF_RENDER("DeferredShadingPass::prepare::SetupCamera");
        m_view = &ctx.view;
        m_projection = &ctx.projection;
        m_sceneDataCB->viewPos = ctx.cameraPosition;
    }

    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    m_depthTexture = ctx.outlineDepthTexture;
    m_normalTexture = ctx.outlineNormalTexture;

    PERF_RENDER("DeferredShadingPass::prepare::UploadSceneDataCB");
    m_sceneDataCBAddress = ctx.ringBuffer->allocate(m_sceneDataCB.get(), sizeof(SceneDataCB));

    m_meshRenderers = app->getModuleScene()->getVisibleForwardMeshRenderers(RenderMode::TRANSP); //TODO: Change to get only renderers with draw outline
}

void OutlinePass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "OutlinePass");

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_normalTexture->getRTV().cpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_depthTexture->getDSV().cpu;

    commandList->OMSetRenderTargets(1, rtv, FALSE, dsv);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootConstantBufferView(1, m_sceneDataCBAddress);
    commandList->SetGraphicsRootDescriptorTable(4, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);



    for (auto* renderer : m_meshRenderers)
    {
        renderMeshRenderer(commandList, renderer);
    }


    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_depthTexture->getD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);
}

void OutlinePass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParams[5] = {};

    CD3DX12_DESCRIPTOR_RANGE normalRange, sampleRange;
    normalRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
    sampleRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); //Model view projection
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); //Scene data
    rootParams[2].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_ALL); //Outline data
    rootParams[3].InitAsDescriptorTable(1, &normalRange, D3D12_SHADER_VISIBILITY_PIXEL); //Mesh normal
    rootParams[4].InitAsDescriptorTable(1, &sampleRange, D3D12_SHADER_VISIBILITY_PIXEL); //Texture samples

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature, error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void OutlinePass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"OutlineVS.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"OutlinePS.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void OutlinePass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer)
{
    GameObject* owner = renderer->getOwner();
    if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
    {
        return;
    }

    if (!renderer->isActive())
    {
        return;
    }

    Transform* transform = renderer->getTransform();

    const auto& mesh = renderer->getMesh();
    if (mesh.get() == nullptr)
    {
        return;
    }

    const auto& submeshes = mesh->getSubmeshes();
    const auto& materials = renderer->getMaterials();

    if (materials.size() != submeshes.size())
    {
        return;
    }

    const Skin* skin = renderer->getSkin();

    const VertexBuffer* gpuSkinnedVB = skin ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* cpuSkinnedVB = skin && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;

    const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

    const bool useGpuSkinnedVB = (gpuSkinnedVB != nullptr);
    const bool useCpuSkinnedVB = (!useGpuSkinnedVB && cpuSkinnedVB != nullptr);
    const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

    const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : (useCpuSkinnedVB ? cpuSkinnedVB : staticVB);

    if (!activeVB)
    {
        return;
    }

    Matrix global = transform->getGlobalMatrix();
    Matrix mvp = useWorldSpaceSkinnedVB ? (*m_view * *m_projection).Transpose() : (global * *m_view * *m_projection).Transpose();

    commandList->SetGraphicsRootConstantBufferView(0, app->getModuleRender()->allocateInRingBuffer(&mvp, sizeof(Matrix)));

    for (int i = 0; i < submeshes.size(); i++)
    {
        const auto& material = materials.at(i).get();

        OutlineData outlineData{};
        outlineData.model = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getGlobalMatrix().Transpose();
        outlineData.normalMat = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getNormalMatrix().Transpose();
        commandList->SetGraphicsRootConstantBufferView(2, app->getModuleRender()->allocateInRingBuffer(&outlineData, sizeof(OutlineData)));

        commandList->SetGraphicsRootDescriptorTable(3, material->getNormal());

        D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
        commandList->IASetVertexBuffers(0, 1, &vbv);

        if (mesh->hasIndexBuffer())
        {
            //PERF_RENDER("MeshRendererPass::renderMesh::DrawIndexed");
            D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();
            commandList->IASetIndexBuffer(&ibv);

            commandList->DrawIndexedInstanced(submeshes.at(i).indexCount, 1, submeshes.at(i).indexStart, 0, 0);
        }
    }
}
