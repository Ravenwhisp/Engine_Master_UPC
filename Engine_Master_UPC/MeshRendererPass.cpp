#include "Globals.h"
#include "MeshRendererPass.h"

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

#include "SimpleMath.h"
#include <d3dcompiler.h>
#include "PlatformHelpers.h"
#include "OptickProfiler.h"

MeshRendererPass::MeshRendererPass(ComPtr<ID3D12Device4> device): m_device(device)
{
	m_lighting = std::make_unique<SceneLightingSettings>();
	m_sceneDataCB = std::make_unique<SceneDataCB>();

    m_lighting->ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting->ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[12] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange, irradianceRange, brdfRange, sampRange, prefilteredRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
    irradianceRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
    prefilteredRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);
    brdfRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[4].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[5].InitAsDescriptorTable(1, &irradianceRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[6].InitAsDescriptorTable(1, &prefilteredRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[7].InitAsDescriptorTable(1, &brdfRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[8].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[9].InitAsShaderResourceView(11); // t11
    rootParameters[10].InitAsShaderResourceView(12); // t12
    rootParameters[11].InitAsShaderResourceView(13); // t13

    rootSignatureDesc.Init(12, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    CreateUploadBuffer(m_directionalBuffer, sizeof(GPUDirectionalLight) * LightDefaults::MAX_DIRECTIONAL_LIGHTS);
    CreateUploadBuffer(m_pointBuffer, sizeof(GPUPointLight) * LightDefaults::MAX_POINT_LIGHTS);
    CreateUploadBuffer(m_spotBuffer, sizeof(GPUSpotLight) * LightDefaults::MAX_SPOT_LIGHTS);

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

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc = { 1,0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

}


void MeshRendererPass::prepare(const RenderContext& ctx)
{
    PERF_RENDER("MeshRendererPass::prepare");

    {
        PERF_RENDER("MeshRendererPass::prepare::SetupCamera");
        m_view = &ctx.view;
        m_projection = &ctx.projection;
        m_sceneDataCB->viewPos = ctx.cameraPosition;
    }

    {
        PERF_RENDER("MeshRendererPass::prepare::GetVisibleMeshRenderers");
        m_meshRenderers = app->getModuleScene()->getVisibleMeshRenderers();
    }

    {
        PERF_RENDER("MeshRendererPass::prepare::UploadSceneDataCB");
        m_sceneDataCBAddress = ctx.ringBuffer->allocate(
            m_sceneDataCB.get(),
            sizeof(SceneDataCB),
            app->getModuleD3D12()->getCurrentFrame());
    }

    {
        PERF_RENDER("MeshRendererPass::prepare::PackLights");
        m_packedLights = packLightsForGPU(
            app->getModuleScene()->getLightComponents(),
            m_lighting->ambientColor,
            m_lighting->ambientIntensity);
    }

    {
        LightsInfo info{};
        info.ambientColor = m_packedLights.ambientColor;
        info.ambientIntensity = m_packedLights.ambientIntensity;
        info.directionalCount = m_packedLights.directionalCount;
        info.pointCount = m_packedLights.pointCount;
        info.spotCount = m_packedLights.spotCount;

        m_lightsAddress = ctx.ringBuffer->allocate(&info, sizeof(LightsInfo), app->getModuleD3D12()->getCurrentFrame());
    }
}

void MeshRendererPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    void* mapped;

    // directional
    m_directionalBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, m_packedLights.directional.data(),
        m_packedLights.directional.size() * sizeof(GPUDirectionalLight));
    m_directionalBuffer->Unmap(0, nullptr);

    // point
    m_pointBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, m_packedLights.point.data(),
        m_packedLights.point.size() * sizeof(GPUPointLight));
    m_pointBuffer->Unmap(0, nullptr);

    // spot
    m_spotBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, m_packedLights.spot.data(),
        m_packedLights.spot.size() * sizeof(GPUSpotLight));
    m_spotBuffer->Unmap(0, nullptr);


    // Bind root signature (must be set before any draw calls)
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    //Set input assembler
    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootConstantBufferView(1, m_sceneDataCBAddress);

    commandList->SetGraphicsRootConstantBufferView(3, m_lightsAddress);

    commandList->SetGraphicsRootDescriptorTable(8, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));

    commandList->SetGraphicsRootShaderResourceView(9, m_directionalBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootShaderResourceView(10, m_pointBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootShaderResourceView(11, m_spotBuffer->GetGPUVirtualAddress());

    renderMesh(commandList);
}


void MeshRendererPass::renderMesh(ID3D12GraphicsCommandList* commandList)
{
    m_trianglesCount = 0;
    m_meshCount = 0;

    PERF_RENDER("MeshRendererPass::renderMesh");

    for (const auto& renderer : m_meshRenderers)
    {
        {
            PERF_RENDER("MeshRendererPass::renderMesh::RendererValidation");

            GameObject* owner = renderer->getOwner();
            if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
            {
                continue;
            }

            if (!renderer->isActive())
            {
                continue;
            }
        }

        Transform* transform = renderer->getTransform();

        const auto& mesh = renderer->getMesh();
        if (mesh.get() == nullptr)
            continue;

        const auto& submeshes = mesh->getSubmeshes();
        const auto& materials = renderer->getMaterials();

        if (materials.size() != submeshes.size())
            continue;

        {
            PERF_RENDER("MeshRendererPass::renderMesh::VertexBufferSelection");

            const VertexBuffer* gpuSkinnedVB = renderer->getCurrentGpuSkinnedVertexBuffer();
            const VertexBuffer* cpuSkinnedVB = renderer->isCpuSkinningFallbackEnabled() ? renderer->getCpuSkinnedVertexBuffer() : nullptr;
            const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

            const bool useGpuSkinnedVB = (gpuSkinnedVB != nullptr);
            const bool useCpuSkinnedVB = (!useGpuSkinnedVB && cpuSkinnedVB != nullptr);
            const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

            const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : (useCpuSkinnedVB ? cpuSkinnedVB : staticVB);

            if (!activeVB)
                continue;

            Matrix global = transform->getGlobalMatrix();
            Matrix mvp = useWorldSpaceSkinnedVB ? (*m_view * *m_projection).Transpose() : (global * *m_view * *m_projection).Transpose();

            commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

            {
                PERF_RENDER("MeshRendererPass::renderMesh::SubmeshLoop");

                for (int i = 0; i < submeshes.size(); i++)
                {
                    m_trianglesCount += submeshes[i].indexCount / 3;
                    m_meshCount++;

                    const auto& material = materials.at(i).get();

                    {
                        PERF_RENDER("MeshRendererPass::renderMesh::ModelDataUpload");
                        ModelData modelData{};
                        modelData.model = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getGlobalMatrix().Transpose();
                        modelData.normalMat = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getNormalMatrix().Transpose();
                        modelData.material = material->getMaterial();

                        commandList->SetGraphicsRootConstantBufferView( 2, app->getModuleRender()->allocateInRingBuffer(&modelData, sizeof(ModelData))
                        );
                    }

                    {
                        PERF_RENDER("MeshRendererPass::renderMesh::BindMaterial");
                        commandList->SetGraphicsRootDescriptorTable(4, material->getTableGPUHandle());
                        commandList->SetGraphicsRootDescriptorTable(5, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getIrradiance()->getSRV().gpu);
                        commandList->SetGraphicsRootDescriptorTable(6, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getEnvironment()->getSRV().gpu);
                        commandList->SetGraphicsRootDescriptorTable(7, app->getModuleResources()->getEnvironmentBrdfTexture()->getSRV().gpu);

                        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                        D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
                        commandList->IASetVertexBuffers(0, 1, &vbv);
                    }

                    if (mesh->hasIndexBuffer())
                    {
                        PERF_RENDER("MeshRendererPass::renderMesh::DrawIndexed");
                        D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();
                        commandList->IASetIndexBuffer(&ibv);
                        commandList->DrawIndexedInstanced(submeshes.at(i).indexCount, 1, submeshes.at(i).indexStart, 0, 0);
                    }
                }
            }
        }
    }
}

PackedLights MeshRendererPass::packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const
{
    PackedLights result{};
    result.ambientColor = ambientColor;
    result.ambientIntensity = ambientIntensity;

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
            if (result.directional.size() < LightDefaults::MAX_DIRECTIONAL_LIGHTS)
            {
                GPUDirectionalLight l{};
                l.direction = fwd;
                l.color = common.color;
                l.intensity = common.intensity;
                result.directional.push_back(l);
            }
            break;

        case LightType::POINT:
            if (result.point.size() < LightDefaults::MAX_POINT_LIGHTS)
            {
                GPUPointLight l{};
                l.position = pos;
                l.radius = data.parameters.point.radius;
                l.color = common.color;
                l.intensity = common.intensity;
                result.point.push_back(l);
            }
            break;

        case LightType::SPOT:
            if (result.spot.size() < LightDefaults::MAX_SPOT_LIGHTS)
            {
                GPUSpotLight l{};
                l.position = pos;
                l.direction = fwd;
                l.radius = data.parameters.spot.radius;
                l.color = common.color;
                l.intensity = common.intensity;
                l.cosineInnerAngle = std::cos(XMConvertToRadians(data.parameters.spot.innerAngleDegrees));
                l.cosineOuterAngle = std::cos(XMConvertToRadians(data.parameters.spot.outerAngleDegrees));
                result.spot.push_back(l);
            }
            break;
        }
    }

    result.directionalCount = (uint32_t)result.directional.size();
    result.pointCount = (uint32_t)result.point.size();
    result.spotCount = (uint32_t)result.spot.size();

    return result;
}

#pragma region UTILS
void MeshRendererPass::CreateUploadBuffer(ComPtr<ID3D12Resource>& buffer, size_t size)
{
    CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);

    m_device->CreateCommittedResource(
        &heap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&buffer)
    );
}

#pragma endregion
