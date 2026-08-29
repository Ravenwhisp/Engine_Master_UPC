#include "Globals.h"
#include "FlowMapPass.h"

#include "Application.h"
#include "BasicMaterial.h"
#include "BasicMesh.h"
#include "FlowMapComponent.h"
#include "GameObject.h"
#include "GeometryPass.h"
#include "IndexBuffer.h"
#include "MeshRenderer.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "ModuleScene.h"
#include "ModuleRender.h"
#include "ModuleTime.h"
#include "RenderContext.h"
#include "RenderSurface.h"
#include "Skin.h"
#include "Texture.h"
#include "Transform.h"
#include "VertexBuffer.h"

#include <d3dcompiler.h>
#include <PlatformHelpers.h>

FlowMapPass::FlowMapPass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
    createRootSignature();
    createPipelineState();
}

void FlowMapPass::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER params[6]{};
    CD3DX12_DESCRIPTOR_RANGE materialRange, samplerRange, flowRange;

    materialRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
    samplerRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);
    flowRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 13, 0);

    params[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    params[1].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    params[2].InitAsDescriptorTable(1, &materialRange, D3D12_SHADER_VISIBILITY_PIXEL);
    params[3].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);
    params[4].InitAsConstantBufferView(6, 0, D3D12_SHADER_VISIBILITY_ALL);
    params[5].InitAsDescriptorTable(1, &flowRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC desc;
    desc.Init(_countof(params), params, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
        &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(),
        signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void FlowMapPass::createPipelineState()
{
    ComPtr<ID3DBlob> vs;
    ComPtr<ID3DBlob> ps;
    ThrowIfFailed(D3DReadFileToBlob(L"FlowMapVS.cso", &vs));
    ThrowIfFailed(D3DReadFileToBlob(L"FlowMapPS.cso", &ps));

    D3D12_INPUT_ELEMENT_DESC input[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthEnable = TRUE;
    depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    depth.StencilEnable = TRUE;
    depth.StencilReadMask = 0xFF;
    depth.StencilWriteMask = 0;
    depth.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    depth.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depth.BackFace = depth.FrontFace;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.InputLayout = { input, _countof(input) };
    desc.pRootSignature = m_rootSignature.Get();
    desc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
    desc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.FrontCounterClockwise = TRUE;
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.DepthStencilState = depth;
    desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    desc.SampleMask = UINT_MAX;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = GeometryPass::GBUFFER_COUNT;
    for (UINT i = 0; i < GeometryPass::GBUFFER_COUNT; ++i)
        desc.RTVFormats[i] = GeometryPass::GBUFFER_FORMATS[i];
    desc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));
}

void FlowMapPass::prepare(const RenderContext& ctx)
{
    m_view = &ctx.view;
    m_projection = &ctx.projection;
    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;
    m_renderSurface = &ctx.renderSurface;
    m_meshRenderers = app->getModuleScene()->getVisibleForwardMeshRenderers(RenderMode::FLOW_MAP);

#ifdef _DEBUG
    const uint32_t frame = app->getModuleTime()->frameCount();
    if ((frame % 60u) == 0u)
        DirectX::DebugTrace("[FlowMap] pass renderers=%zu\n", m_meshRenderers.size());
#endif
}

void FlowMapPass::transitionGBuffer(ID3D12GraphicsCommandList4* commandList,
    D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) const
{
    CD3DX12_RESOURCE_BARRIER barriers[GeometryPass::GBUFFER_COUNT];
    for (UINT i = 0; i < GeometryPass::GBUFFER_COUNT; ++i)
    {
        auto texture = m_renderSurface->getTexture(GeometryPass::kSlots[i]);
        barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
            texture->getD3D12Resource().Get(), before, after);
    }
    commandList->ResourceBarrier(GeometryPass::GBUFFER_COUNT, barriers);
}

void FlowMapPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (!m_renderSurface || m_meshRenderers.empty())
        return;

    transitionGBuffer(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[GeometryPass::GBUFFER_COUNT];
    for (UINT i = 0; i < GeometryPass::GBUFFER_COUNT; ++i)
        rtvs[i] = m_renderSurface->getTexture(GeometryPass::kSlots[i])->getRTV(0).cpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_renderSurface->getTexture(RenderSurface::DEPTH_STENCIL)->getDSV().cpu;
    commandList->OMSetRenderTargets(GeometryPass::GBUFFER_COUNT, rtvs, FALSE, &dsv);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);
    commandList->OMSetStencilRef(static_cast<UINT>(RenderMode::FLOW_MAP));
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap()
    };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    commandList->SetGraphicsRootDescriptorTable(3,
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
            .getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));
    commandList->SetGraphicsRootDescriptorTable(5,
        app->getModuleResources()->getEnvironmentBrdfTexture()->getSRV().gpu);

    for (MeshRenderer* renderer : m_meshRenderers)
        renderMeshRenderer(commandList, renderer);

    transitionGBuffer(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void FlowMapPass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList,
    MeshRenderer* renderer)
{
    GameObject* owner = renderer->getOwner();
    if (!owner || !owner->IsActiveInWindowHierarchy() || !renderer->isActive())
        return;

    auto* flow = owner->GetComponentAs<FlowMapComponent>(ComponentType::FLOW_MAP);
    if (!flow || !flow->isActive())
        return;

    const auto& mesh = renderer->getMesh();
    if (!mesh)
        return;
    const auto& submeshes = mesh->getSubmeshes();
    const auto& materials = renderer->getMaterials();
    if (submeshes.size() != materials.size())
        return;

    const Skin* skin = renderer->getSkin();
    const VertexBuffer* gpuSkin = skin ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
    const VertexBuffer* cpuSkin = skin && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;
    const bool worldSpaceSkin = gpuSkin || cpuSkin;
    const VertexBuffer* vb = gpuSkin ? gpuSkin : (cpuSkin ? cpuSkin : mesh->getVertexBuffer().get());
    if (!vb)
        return;

    Transform* transform = renderer->getTransform();
    Matrix mvp = worldSpaceSkin ? (*m_view * *m_projection).Transpose()
        : (transform->getGlobalMatrix() * *m_view * *m_projection).Transpose();
    commandList->SetGraphicsRootConstantBufferView(0,
        app->getModuleRender()->allocateInRingBuffer(&mvp, sizeof(mvp)));

    FlowMapGPUData flowGpu{};
    const FlowMapData data = flow->getData();
    flowGpu.direction = data.direction;
    flowGpu.tiling = data.tiling;
    flowGpu.offset = data.offset;
    flowGpu.strength = data.strength;
    flowGpu.source = data.source;
    flowGpu.enabled = data.enabled;
    flowGpu.technique = data.technique;
    flowGpu.phase = flow->getPhase();
    flowGpu.exaggeration = data.exaggeration;
    flowGpu.textureStrength = data.textureStrength;

#ifdef _DEBUG
    if ((app->getModuleTime()->frameCount() % 60u) == 0u &&
        data.technique == static_cast<uint32_t>(FlowMapTechnique::FLOW_MAP_WATER))
    {
        DirectX::DebugTrace("[FlowMap] water phase=%.3f source=%u texture=%s\n",
            flowGpu.phase, data.source, flow->getTexture() ? "YES" : "NO");
    }
#endif

#ifdef _DEBUG
    const uint32_t frame = app->getModuleTime()->frameCount();
    if ((frame % 60u) == 0u)
    {
        DirectX::DebugTrace("[FlowMap] draw renderer=%p offset=(%.4f, %.4f) tiling=(%.3f, %.3f) enabled=%u\n",
            static_cast<void*>(renderer), data.offset.x, data.offset.y,
            data.tiling.x, data.tiling.y, data.enabled);
    }
#endif
    if (data.technique == static_cast<uint32_t>(FlowMapTechnique::FLOW_MAP_WATER) &&
        data.source == static_cast<uint32_t>(FlowMapSource::TEXTURE) && flow->getTexture())
        commandList->SetGraphicsRootDescriptorTable(5, flow->getTexture()->getSRV().gpu);
    else
        flowGpu.source = static_cast<uint32_t>(FlowMapSource::DIRECTION);
    commandList->SetGraphicsRootConstantBufferView(4,
        app->getModuleRender()->allocateInRingBuffer(&flowGpu, sizeof(flowGpu)));

    for (size_t i = 0; i < submeshes.size(); ++i)
    {
        ModelData model{};
        model.model = worldSpaceSkin ? Matrix::Identity.Transpose() : transform->getGlobalMatrix().Transpose();
        model.normalMat = worldSpaceSkin ? Matrix::Identity.Transpose() : transform->getNormalMatrix().Transpose();
        model.material = materials[i]->getMaterial();
        commandList->SetGraphicsRootConstantBufferView(1,
            app->getModuleRender()->allocateInRingBuffer(&model, sizeof(model)));
        commandList->SetGraphicsRootDescriptorTable(2, materials[i]->getTableGPUHandle());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        D3D12_VERTEX_BUFFER_VIEW vbv = vb->getVertexBufferView();
        commandList->IASetVertexBuffers(0, 1, &vbv);
        if (mesh->hasIndexBuffer())
        {
            auto ibv = mesh->getIndexBuffer()->getIndexBufferView();
            commandList->IASetIndexBuffer(&ibv);
            commandList->DrawIndexedInstanced(submeshes[i].indexCount, 1,
                submeshes[i].indexStart, 0, 0);
        }
    }
}
