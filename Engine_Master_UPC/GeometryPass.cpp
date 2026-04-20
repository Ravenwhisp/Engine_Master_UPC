#include "Globals.h"
#include "GeometryPass.h"

#include "MeshRenderer.h"
#include "Transform.h"
#include "RenderContext.h"
#include <SceneDataCB.h>
#include "GameObject.h"


#include "Application.h"
#include "ModuleScene.h"
#include "ModuleD3D12.h"
#include "ModuleRender.h"
#include "ModuleResources.h"

#include "RingBuffer.h"
#include <d3dcompiler.h>
#include <PlatformHelpers.h>

GeometryPass::GeometryPass(ComPtr<ID3D12Device4> device): m_device(device)
{
    createRootSignature();
    createPipelineState();
    createGBufferSurface();
}

void GeometryPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER   rootParams[5] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange, sampRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParams[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParams[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParams[3].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[4].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> sigBlob, errorBlob;
    DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob));
    DXCall(m_device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void GeometryPass::createPipelineState()
{
    ComPtr<ID3DBlob> vsBlob, psBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"GBufferVS.cso", &vsBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"GBufferPS.cso", &psBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = GBUFFER_COUNT;
    for (UINT i = 0; i < GBUFFER_COUNT; ++i)
    {
        psoDesc.RTVFormats[i] = GBUFFER_FORMATS[i];
    }

    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void GeometryPass::createGBufferSurface()
{
    m_gbufferSurface = new RenderSurface();

    for (UINT i = 0; i < GBUFFER_COUNT; ++i)
    {
        m_gbufferSurface->attachTexture(kSlots[i], std::make_shared<Texture>(app->getModuleResources()->createGBuffer(1, 1, GBUFFER_FORMATS[0])));
    }

    m_gbufferSurface->attachTexture(RenderSurface::DEPTH_STENCIL, std::make_shared<Texture>(app->getModuleResources()->createDepthBuffer(1, 1)));
}

void GeometryPass::prepare(const RenderContext& ctx)
{
    m_view = &ctx.view;
    m_projection = &ctx.projection;

    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    // Collect visible mesh renderers
    m_meshRenderers = app->getModuleScene()->getMeshRenderers();

    // Upload SceneDataCB (camera position) to the ring buffer
    SceneDataCB sceneData{};
    sceneData.viewPos = ctx.cameraPosition;

    m_sceneDataCBAddress = ctx.ringBuffer->allocate(&sceneData, sizeof(SceneDataCB), app->getModuleD3D12()->getCurrentFrame());

    if(ctx.renderSurface.getSize() != m_gbufferSurface->getSize())
    {
		m_gbufferSurface->resize(ctx.renderSurface.getSize());
	}
}

void GeometryPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "Geometry Pass");

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[GBUFFER_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

    transitionAndClearTargets(commandList, rtvHandles, &dsvHandle);
    setupPipelineAndHeaps(commandList);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (auto* renderer : m_meshRenderers)
    {
        renderMeshRenderer(commandList, renderer);
    }

    transitionGBuffer(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    END_EVENT(commandList);
}

void GeometryPass::transitionAndClearTargets(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle) const
{
    transitionGBuffer(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const float clearColour[GBUFFER_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (UINT i = 0; i < GBUFFER_COUNT; ++i)
    {
        rtvHandles[i] = m_gbufferSurface->getTexture(kSlots[i])->getRTV(0).cpu;
        commandList->ClearRenderTargetView(rtvHandles[i], clearColour, 0, nullptr);
    }

    *dsvHandle = m_gbufferSurface->getTexture(RenderSurface::DEPTH_STENCIL)->getDSV().cpu;
    commandList->ClearDepthStencilView(*dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(GBUFFER_COUNT, rtvHandles, FALSE, dsvHandle);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);
}

void GeometryPass::setupPipelineAndHeaps(ID3D12GraphicsCommandList4* commandList) const
{
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootConstantBufferView(1, m_sceneDataCBAddress);
    commandList->SetGraphicsRootDescriptorTable(4, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));
}

void GeometryPass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer) const
{
    const GameObject* owner = renderer->getOwner();
    if (!owner || !owner->IsActiveInWindowHierarchy() || !renderer->isActive())
        return;

    const auto& mesh = renderer->getMesh();
    if (!mesh) return;

    const auto& submeshes = mesh->getSubmeshes();
    const auto& materials = renderer->getMaterials();
    if (materials.size() != submeshes.size()) return;

    const VertexBuffer* gpuVB = renderer->getCurrentGpuSkinnedVertexBuffer();
    const VertexBuffer* cpuVB = renderer->isCpuSkinningFallbackEnabled() ? renderer->getCpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

    const bool useWorldSpace = (gpuVB != nullptr) || (cpuVB != nullptr);
    const VertexBuffer* activeVB = gpuVB ? gpuVB : (cpuVB ? cpuVB : staticVB);
    if (!activeVB) return;

    Transform* transform = renderer->getTransform();
    Matrix mvp = useWorldSpace ? (*m_view * *m_projection).Transpose() : (transform->getGlobalMatrix() * *m_view * *m_projection).Transpose();

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);

    D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    for (int s = 0; s < (int)submeshes.size(); ++s)
    {
        auto* material = materials[s].get();

        ModelData modelData{};
        modelData.model = useWorldSpace ? Matrix::Identity.Transpose() : transform->getGlobalMatrix().Transpose();
        modelData.normalMat = useWorldSpace ? Matrix::Identity.Transpose() : transform->getNormalMatrix().Transpose();
        modelData.material = material->getMaterial();

        commandList->SetGraphicsRootConstantBufferView(2, app->getModuleRender()->allocateInRingBuffer(&modelData, sizeof(ModelData)));

        commandList->SetGraphicsRootDescriptorTable(3, material->getTexture()->getSRV().gpu);

        if (mesh->hasIndexBuffer())
        {
            D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();
            commandList->IASetIndexBuffer(&ibv);
            commandList->DrawIndexedInstanced(submeshes[s].indexCount, 1, submeshes[s].indexStart, 0, 0);
        }
    }
}


void GeometryPass::transitionGBuffer(ID3D12GraphicsCommandList4* cmdList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) const
{
    CD3DX12_RESOURCE_BARRIER barriers[GBUFFER_COUNT];
    for (UINT i = 0; i < GBUFFER_COUNT; ++i)
    {
        barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferSurface->getTexture(kSlots[i])->getD3D12Resource().Get(), before, after);
    }
    cmdList->ResourceBarrier(GBUFFER_COUNT, barriers);
}