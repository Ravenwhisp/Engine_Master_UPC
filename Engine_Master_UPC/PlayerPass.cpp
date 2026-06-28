#include "Globals.h"
#include "PlayerPass.h"
#include "Application.h"

#include "BasicMaterial.h"
#include "RenderContext.h"
#include "SceneDataCB.h"
#include "LightComponent.h"
#include "RingBuffer.h"
#include "SceneLightingSettings.h"
#include "GameObject.h"
#include "Transform.h"
#include "SkyboxPass.h"
#include "Skybox.h"
#include "IndexBuffer.h"
#include "PlayerRenderBufferComponent.h"

#include "ModuleDescriptors.h"
#include "ModuleScene.h"
#include "ModuleD3d12.h"
#include "ModuleRender.h"
#include "ModuleResources.h"

#include "PlatformHelpers.h"

#include <d3dcompiler.h>


PlayerPass::PlayerPass(ComPtr<ID3D12Device4> device)
{
	m_device = device;
    m_lighting = std::make_unique<SceneLightingSettings>();
    m_sceneDataCB = std::make_unique<SceneDataCB>();

	createRootSignature();
	createPipelineState();
}

void PlayerPass::createRootSignature()
{
	CD3DX12_ROOT_PARAMETER		rootParams[12] = {};
	CD3DX12_DESCRIPTOR_RANGE	srvRange, irradianceRange, brdfRange, sampRange, prefilteredRange, shadowMapRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
    irradianceRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
    brdfRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);
    prefilteredRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);
    shadowMapRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11, 0);

    rootParams[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); //Model view projection
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); //Scene data
    rootParams[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL); //Lights
    rootParams[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_PIXEL); //Shadows
    rootParams[4].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_ALL); //Model data
    rootParams[5].InitAsConstantBufferView(5, 0, D3D12_SHADER_VISIBILITY_PIXEL); //Player data
    rootParams[6].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL); //Mesh textures
    rootParams[7].InitAsDescriptorTable(1, &irradianceRange, D3D12_SHADER_VISIBILITY_PIXEL); //Irradiance texture
    rootParams[8].InitAsDescriptorTable(1, &prefilteredRange, D3D12_SHADER_VISIBILITY_PIXEL); //Prefiltered texture
    rootParams[9].InitAsDescriptorTable(1, &brdfRange, D3D12_SHADER_VISIBILITY_PIXEL); //Brdf texture
    rootParams[10].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL); //Texture samples
    rootParams[11].InitAsDescriptorTable(1, &shadowMapRange, D3D12_SHADER_VISIBILITY_PIXEL); //Shadow map texture

	CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> sigBlob, errorBlob;
	DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob));
	DXCall(m_device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void PlayerPass::createPipelineState()
{
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PlayerVertexShader.cso", &vertexShaderBlob));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PlayerPixelShader.cso", &pixelShaderBlob));

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // Describe the depth stencil state
    D3D12_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    dsDesc.StencilEnable = TRUE;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0x00;
    dsDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    dsDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    dsDesc.BackFace = dsDesc.FrontFace;

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = dsDesc; 
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc = { 1,0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void PlayerPass::prepare(const RenderContext& ctx)
{
    m_view = &ctx.view;
    m_projection = &ctx.projection;
    m_sceneDataCB->viewPos = ctx.cameraPosition;

    m_sceneDataCBAddress = ctx.ringBuffer->allocate(m_sceneDataCB.get(), sizeof(SceneDataCB), app->getModuleD3D12()->getCurrentFrame());

    m_viewport = ctx.viewport;
    m_scissorRect = ctx.scissorRect;

    // Collect visible mesh renderers
    m_meshRenderers = app->getModuleScene()->getVisibleForwardMeshRenderers(RenderMode::PLAYER);

    // Upload SceneDataCB (camera position) to the ring buffer
    SceneDataCB sceneData{};
    sceneData.viewPos = ctx.cameraPosition;

    m_renderSurface = &ctx.renderSurface;

    GPULightsConstantBuffer lightsCB{};
    lightsCB = packLightsForGPU(app->getModuleScene()->getLightComponents(),m_lighting->ambientColor,m_lighting->ambientIntensity);

    m_lightsAddress = ctx.ringBuffer->allocate( &lightsCB, sizeof(GPULightsConstantBuffer), app->getModuleD3D12()->getCurrentFrame());

    m_hasShadowData = ctx.shadowData != nullptr;

    if (m_hasShadowData)
    {
        m_shadowCBAddress = ctx.shadowData->shadowCBAddress;
        m_shadowMapSRV = ctx.shadowData->shadowMapSRV;
    }
    else
    {
        m_shadowCBAddress = 0;
        m_shadowMapSRV = {};
    }
}

void PlayerPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    BEGIN_EVENT(commandList, "PlayerPass");

    auto colorTex = m_renderSurface->getTexture(RenderSurface::COMPOSITE);
    auto depthStencilTex = m_renderSurface->getTexture(RenderSurface::DEPTH_STENCIL);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = colorTex->getRTV(0).cpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthStencilTex->getDSV().cpu;
    
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->OMSetStencilRef(static_cast<UINT>(RenderMode::PLAYER));

    commandList->SetGraphicsRootConstantBufferView(1, m_sceneDataCBAddress);
    commandList->SetGraphicsRootConstantBufferView(2, m_lightsAddress);

    commandList->SetGraphicsRootDescriptorTable(7, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getIrradiance()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(8, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getEnvironment()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(9, app->getModuleResources()->getEnvironmentBrdfTexture()->getSRV().gpu);
    commandList->SetGraphicsRootDescriptorTable(10, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));

    if (m_hasShadowData && m_shadowCBAddress != 0 && m_shadowMapSRV.ptr != 0)
    {
        commandList->SetGraphicsRootConstantBufferView(3, m_shadowCBAddress);
        commandList->SetGraphicsRootDescriptorTable(11, m_shadowMapSRV);
    }

    for (auto* renderer : m_meshRenderers)
    {
        renderMeshRenderer(commandList, renderer);
    }

    END_EVENT(commandList);
}


void PlayerPass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer)
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

    PlayerRenderBufferComponent* playerRenderBufferComponent = static_cast<PlayerRenderBufferComponent*>(owner->GetComponent(ComponentType::PLAYER_RENDER_BUFFER));
    PlayerRenderBufferComponent::PlayerRenderBuffer playerRenderBuffer{};
    if (playerRenderBufferComponent)
    {
        playerRenderBuffer.damageHighlight = playerRenderBufferComponent->getDamageHighlight();
        playerRenderBuffer.damageHighlightData = playerRenderBufferComponent->getDamageHighlightData();
    }
    commandList->SetGraphicsRootConstantBufferView(5, app->getModuleRender()->allocateInRingBuffer(&playerRenderBuffer, sizeof(PlayerRenderBufferComponent::PlayerRenderBuffer)));

    Matrix global = transform->getGlobalMatrix();
    Matrix mvp = useWorldSpaceSkinnedVB ? (*m_view * *m_projection).Transpose() : (global * *m_view * *m_projection).Transpose();

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);

    for (int i = 0; i < submeshes.size(); i++)
    {
        const auto& material = materials.at(i).get();
        
        ModelData modelData{};
        modelData.model = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getGlobalMatrix().Transpose();
        modelData.normalMat = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getNormalMatrix().Transpose();
        modelData.material = material->getMaterial();

        commandList->SetGraphicsRootConstantBufferView(4, app->getModuleRender()->allocateInRingBuffer(&modelData, sizeof(ModelData)));

        commandList->SetGraphicsRootDescriptorTable(6, material->getTableGPUHandle());

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

GPULightsConstantBuffer PlayerPass::packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const
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