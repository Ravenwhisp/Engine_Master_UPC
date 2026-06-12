#include "Globals.h"
#include "DeferredShadingPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "ModuleRender.h"
#include "ModuleScene.h"
#include "SkyBoxPass.h"

#include "Scene.h"
#include "SceneLightingSettings.h"
#include "SceneDataCB.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "LightComponent.h"
#include "RingBuffer.h"
#include "VertexBuffer.h"
#include "Texture.h"
#include "BasicMesh.h"
#include "SkyBox.h"
#include "RenderSurface.h"

#include "SimpleMath.h"
#include <d3dcompiler.h>
#include "PlatformHelpers.h"
#include "OptickProfiler.h"

#include "GeometryPass.h"

#include <iostream>

DeferredShadingPass::DeferredShadingPass(ComPtr<ID3D12Device4> device): m_device(device)
{
	m_lighting = std::make_unique<SceneLightingSettings>();
	m_sceneDataCB = std::make_unique<SceneDataCB>();

    m_lighting->ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting->ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[7] = {};
    CD3DX12_DESCRIPTOR_RANGE gBufferRange, irradianceRange, brdfRange, sampRange, prefilteredRange;

    gBufferRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GeometryPass::GBUFFER_COUNT, 0, 0);
    irradianceRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
    prefilteredRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);
    brdfRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // camera pos
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // lights
    rootParameters[2].InitAsDescriptorTable(1, &gBufferRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, &irradianceRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[4].InitAsDescriptorTable(1, &prefilteredRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[5].InitAsDescriptorTable(1, &brdfRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[6].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));


    #if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
        UINT compileFlags = 0;
    #endif

    // Load the vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"LightPixelShader.cso", &pixelShaderBlob));

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = m_rootSignature.Get();
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
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}


void DeferredShadingPass::prepare(const RenderContext& ctx)
{
    PERF_RENDER("DeferredShadingPass::prepare");

    {
        PERF_RENDER("DeferredShadingPass::prepare::SetupCamera");
        m_view = &ctx.view;
        m_projection = &ctx.projection;
        m_sceneDataCB->viewPos = ctx.cameraPosition;
    }

    {
        PERF_RENDER("DeferredShadingPass::prepare::UploadSceneDataCB");
        m_sceneDataCBAddress = ctx.ringBuffer->allocate(
            m_sceneDataCB.get(),
            sizeof(SceneDataCB),
            app->getModuleD3D12()->getCurrentFrame());
    }

    GPULightsConstantBuffer lightsCB{};
    {
        PERF_RENDER("DeferredShadingPass::prepare::PackLights");
        lightsCB = packLightsForGPU(
            app->getModuleScene()->getLightComponents(),
            m_lighting->ambientColor,
            m_lighting->ambientIntensity);
    }

    {
        PERF_RENDER("DeferredShadingPass::prepare::UploadLightsCB");
        m_lightsAddress = ctx.ringBuffer->allocate(
            &lightsCB,
            sizeof(GPULightsConstantBuffer),
            app->getModuleD3D12()->getCurrentFrame());
    }

    m_renderSurface = &ctx.renderSurface;
    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;
}

void DeferredShadingPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    // No need to transition from/to any states since all resources are already and will end up in the states we want them to


    auto colorTex = m_renderSurface->getTexture(RenderSurface::COMPOSITE);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = colorTex->getRTV(0).cpu;
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    // Bind root signature (must be set before any draw calls)
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    //Set input assembler
    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootConstantBufferView(0, m_sceneDataCBAddress);

    commandList->SetGraphicsRootConstantBufferView(1, m_lightsAddress);

    commandList->SetGraphicsRootDescriptorTable(2, m_renderSurface->getDescriptorTableGPUHandle());

    commandList->SetGraphicsRootDescriptorTable(3, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getIrradiance()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(4, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getEnvironment()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(5, app->getModuleResources()->getEnvironmentBrdfTexture()->getSRV().gpu);

    commandList->SetGraphicsRootDescriptorTable(6, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));

    commandList->DrawInstanced(3, 1, 0, 0);
}

GPULightsConstantBuffer DeferredShadingPass::packLightsForGPU(
    const std::vector<LightComponent*>& lights,
    const Vector3& ambientColor,
    float ambientIntensity) const
{
    GPULightsConstantBuffer cb{};
    cb.ambientColor = ambientColor;
    cb.ambientIntensity = ambientIntensity;

    for (const LightComponent* light : lights)
    {
        if (!light->isActive())
            continue;

        const GameObject* owner = light->getOwner();
        if (!owner || !owner->IsActiveInWindowHierarchy())
            continue;

        const Transform* transform = owner->GetTransform();
        if (!transform)
            continue;

        const LightData& data = light->getData();
        const LightCommon& common = data.common;
        const Matrix& world = transform->getGlobalMatrix();
        const Vector3      pos(world._41, world._42, world._43);
        const Vector3      fwd = transform->getForward();

        switch (data.type)
        {
        case LightType::DIRECTIONAL:
            if (cb.directionalCount < LightDefaults::MAX_DIRECTIONAL_LIGHTS)
            {
                auto& l = cb.directionalLights[cb.directionalCount++];
                l.direction = fwd;
                l.color = common.color;
                l.intensity = common.intensity;
            }
            break;

        case LightType::POINT:
            if (cb.pointCount < LightDefaults::MAX_POINT_LIGHTS)
            {
                auto& l = cb.pointLights[cb.pointCount++];
                l.position = pos;
                l.radius = data.parameters.point.radius;
                l.color = common.color;
                l.intensity = common.intensity;
            }
            break;

        case LightType::SPOT:
            if (cb.spotCount < LightDefaults::MAX_SPOT_LIGHTS)
            {
                const auto& sp = data.parameters.spot;
                auto& l = cb.spotLights[cb.spotCount++];
                l.position = pos;
                l.direction = fwd;
                l.radius = sp.radius;
                l.color = common.color;
                l.intensity = common.intensity;
                l.cosineInnerAngle = std::cos(XMConvertToRadians(sp.innerAngleDegrees));
                l.cosineOuterAngle = std::cos(XMConvertToRadians(sp.outerAngleDegrees));
            }
            break;

        default: break;
        }
    }

    return cb;
}