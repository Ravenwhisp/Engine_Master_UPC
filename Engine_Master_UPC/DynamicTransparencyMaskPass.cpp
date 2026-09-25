#include "Globals.h"
#include "DynamicTransparencyMaskPass.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleRender.h"
#include "ModuleScene.h"

#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"

#include "OcclusionTargetComponent.h"

#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "BoundingBox.h"

#include "PlatformHelpers.h"

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <cfloat>

DynamicTransparencyMaskPass::DynamicTransparencyMaskPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    createRootSignature();
    createPipelineState();
}

void DynamicTransparencyMaskPass::createRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE occluderDepthRange;
    occluderDepthRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParams[2] = {};
    rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[1].InitAsDescriptorTable(1, &occluderDepthRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void DynamicTransparencyMaskPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"DynamicTransparencyMaskPS.cso", &pixelShaderBlob));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc = { 1, 0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

namespace
{
    // Prefer the body over weapons/attachments. BoundingBox stores mesh bounds,
    // not the current skinned vertex silhouette.
    MeshRenderer* findBodyRenderer(GameObject* object, bool requireSkin)
    {
        if (!object || !object->IsActiveInWindowHierarchy())
            return nullptr;
        auto* renderer = object->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
        if (renderer && renderer->isActive() && renderer->hasMesh() && (!requireSkin || renderer->getSkin()))
            return renderer;
        if (auto* transform = object->GetTransform())
            for (auto* child : transform->getAllChildren())
                if (auto* body = findBodyRenderer(child, requireSkin))
                    return body;
        return nullptr;
    }
}

bool DynamicTransparencyMaskPass::projectPoint(const Vector3& point, Vector3& screen) const
{
    const Matrix viewProjection = *m_view * *m_projection;
    const auto clip = Vector4::Transform(Vector4(point.x, point.y, point.z, 1.0f), viewProjection);
    if (clip.w <= 0.0001f || clip.z < 0.0f || clip.z > clip.w)
        return false;
    screen = Vector3(
        m_viewport.TopLeftX + (clip.x / clip.w * 0.5f + 0.5f) * m_viewport.Width,
        m_viewport.TopLeftY + (-clip.y / clip.w * 0.5f + 0.5f) * m_viewport.Height,
        clip.z / clip.w);
    return true;
}

bool DynamicTransparencyMaskPass::buildRegionForTarget(OcclusionTargetComponent* target, UINT targetIndex)
{
    if (!target || targetIndex >= MAX_DYNAMIC_TRANSPARENCY_TARGETS)
        return false;
    auto* root = target->getOwner();
    if (!root || !root->GetTransform())
        return false;
    const Matrix world = root->GetTransform()->getGlobalMatrix();
    const Vector3 position = Vector3::Transform(Vector3::Zero, world);
    float height = target->getBodyHeight() * Vector3::TransformNormal(Vector3::UnitY, world).Length();
    if (height <= 0.0001f)
    {
        auto* body = findBodyRenderer(root, true);
        if (!body)
            body = findBodyRenderer(root, false);
        if (!body)
            return false;
        const auto* points = body->getBoundingBox().getPoints();
        float minY = FLT_MAX, maxY = -FLT_MAX;
        for (UINT i = 0; i < 8; ++i)
        {
            minY = std::min(minY, points[i].y);
            maxY = std::max(maxY, points[i].y);
        }
        height = maxY - minY;
    }
    if (height <= 0.0001f)
        return false;

    // A rotation-independent body proxy anchored to the movement root. Neither
    // weapon swings nor animated limbs can enlarge or activate this region.
    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
    const float halfWidth = height * 0.22f;
    for (UINT i = 0; i < 8; ++i)
    {
        Vector3 screen;
        const Vector3 corner = position + Vector3(
            (i & 1) ? halfWidth : -halfWidth,
            (i & 2) ? height : 0.0f,
            (i & 4) ? halfWidth : -halfWidth);
        // Disable near-plane-straddling proxies rather than project infinities.
        if (!projectPoint(corner, screen))
            return false;
        minX = std::min(minX, screen.x); maxX = std::max(maxX, screen.x);
        minY = std::min(minY, screen.y); maxY = std::max(maxY, screen.y);
    }

    const float scale = std::max(target->getBubbleScale(), 1.0f);
    m_maskCB.centerRadius[targetIndex] = Vector4((minX + maxX) * 0.5f, (minY + maxY) * 0.5f,
        std::max((maxX - minX) * 0.5f * scale, 1.0f), std::max((maxY - minY) * 0.5f * scale, 1.0f));

    const float margin = height * target->getOcclusionMargin();
    const Matrix inverseView = m_view->Invert();
    Vector3 towardCamera = Vector3::TransformNormal(Vector3::UnitZ, inverseView);
    towardCamera.Normalize(); // Engine view space looks along -Z.
    float cutoffDepth = 0.0f;
    for (UINT probe = 0; probe < DYNAMIC_TRANSPARENCY_PROBES; ++probe)
    {
        const float fraction = target->getAnchorHeight() + (static_cast<float>(probe) - 1.0f) * 0.1f;
        const Vector3 anchor = position + Vector3(0.0f, height * fraction, 0.0f);
        Vector3 screen, biasedScreen;
        if (!projectPoint(anchor, screen) || !projectPoint(anchor + towardCamera * margin, biasedScreen))
            return false;
        const float bias = std::max(screen.z - biasedScreen.z, 0.0000001f);
        m_maskCB.probes[targetIndex * DYNAMIC_TRANSPARENCY_PROBES + probe] = Vector4(screen.x, screen.y, screen.z, bias);
        if (probe == 1)
            cutoffDepth = biasedScreen.z;
    }
    m_maskCB.depthSoftness[targetIndex] = Vector4(cutoffDepth, target->getBubbleSoftness(), 1.0f / scale, 0.0f);
    return true;
}

void DynamicTransparencyMaskPass::prepare(const RenderContext& ctx)
{
    m_renderSurface = &ctx.renderSurface;
    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    m_view = &ctx.view;
    m_projection = &ctx.projection;

    m_maskCB = {};

    UINT targetIndex = 0;

    const auto& targets = app->getModuleScene()->getOcclusionTargetComponents();

    for (OcclusionTargetComponent* target : targets)
    {
        if (target == nullptr || !target->isActive())
            continue;

        GameObject* owner = target->getOwner();

        if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
            continue;

        if (targetIndex >= MAX_DYNAMIC_TRANSPARENCY_TARGETS)
            break;

        if (buildRegionForTarget(target, targetIndex))
            ++targetIndex;
    }

    m_maskCB.settings = DirectX::SimpleMath::Vector4(static_cast<float>(targetIndex), 0.0f, 0.0f, 0.0f);
    m_maskCBAddress = app->getModuleRender()->allocateInRingBuffer(&m_maskCB, sizeof(DynamicTransparencyMaskCB));
}

void DynamicTransparencyMaskPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "DynamicTransparencyMaskPass");

    std::shared_ptr<Texture> occluderDepth = m_renderSurface->getTexture(RenderSurface::OCCLUSION_OCCLUDER_DEPTH);
    std::shared_ptr<Texture> mask = m_renderSurface->getTexture(RenderSurface::DYNAMIC_TRANSPARENCY_MASK);

    if (!occluderDepth || !mask)
    {
        END_EVENT(commandList);
        return;
    }

    CD3DX12_RESOURCE_BARRIER barriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(occluderDepth->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(mask->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
    };

    commandList->ResourceBarrier(_countof(barriers), barriers);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = mask->getRTV(0).cpu;
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootConstantBufferView(0, m_maskCBAddress);
    commandList->SetGraphicsRootDescriptorTable(1, occluderDepth->getSRV().gpu);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    CD3DX12_RESOURCE_BARRIER restoreBarriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(occluderDepth->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
        CD3DX12_RESOURCE_BARRIER::Transition(mask->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    };

    commandList->ResourceBarrier(_countof(restoreBarriers), restoreBarriers);

    END_EVENT(commandList);
}