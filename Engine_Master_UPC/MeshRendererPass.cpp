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

MeshRendererPass::MeshRendererPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
    m_lighting = std::make_unique<SceneLightingSettings>();
    m_sceneDataCB = std::make_unique<SceneDataCB>();

    m_lighting->ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting->ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    CD3DX12_DESCRIPTOR_RANGE directionalRange;
    CD3DX12_DESCRIPTOR_RANGE pointRange;
    CD3DX12_DESCRIPTOR_RANGE spotRange;

    directionalRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11);
    pointRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12);
    spotRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 13);

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
    rootParameters[9].InitAsDescriptorTable(1, &directionalRange);
    rootParameters[10].InitAsDescriptorTable(1, &pointRange);
    rootParameters[11].InitAsDescriptorTable(1, &spotRange);

    rootSignatureDesc.Init(12, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    CreateBuffer(m_directionalBuffer, m_directionalUpload, sizeof(GPUDirectionalLight) * LightDefaults::MAX_DIRECTIONAL_LIGHTS);
    CreateBuffer(m_pointBuffer, m_pointUpload, sizeof(GPUPointLight) * LightDefaults::MAX_POINT_LIGHTS);
    CreateBuffer(m_spotBuffer, m_spotUpload, sizeof(GPUSpotLight) * LightDefaults::MAX_SPOT_LIGHTS);

    DescriptorHeap& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    {
        DescriptorHandle handle = heap.allocate();
        m_directionalHandle = handle.gpu;

        srvDesc.Buffer.NumElements = LightDefaults::MAX_DIRECTIONAL_LIGHTS;
        srvDesc.Buffer.StructureByteStride = sizeof(GPUDirectionalLight);

        m_device->CreateShaderResourceView(m_directionalBuffer.Get(), &srvDesc, handle.cpu);
    }

    {
        DescriptorHandle handle = heap.allocate();
        m_pointHandle = handle.gpu;

        srvDesc.Buffer.NumElements = LightDefaults::MAX_POINT_LIGHTS;
        srvDesc.Buffer.StructureByteStride = sizeof(GPUPointLight);

        m_device->CreateShaderResourceView(m_pointBuffer.Get(), &srvDesc, handle.cpu);
    }

    {
        DescriptorHandle handle = heap.allocate();
        m_spotHandle = handle.gpu;

        srvDesc.Buffer.NumElements = LightDefaults::MAX_SPOT_LIGHTS;
        srvDesc.Buffer.StructureByteStride = sizeof(GPUSpotLight);

        m_device->CreateShaderResourceView(m_spotBuffer.Get(), &srvDesc, handle.cpu);
    }

#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"LightPixelShader.cso", &pixelShaderBlob));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

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
    UpdateBuffer(
        commandList,
        m_directionalBuffer.Get(),
        m_directionalUpload.Get(),
        m_packedLights.directional.data(),
        m_packedLights.directional.size() * sizeof(GPUDirectionalLight),
        sizeof(GPUDirectionalLight) * LightDefaults::MAX_DIRECTIONAL_LIGHTS
    );

    UpdateBuffer(
        commandList,
        m_pointBuffer.Get(),
        m_pointUpload.Get(),
        m_packedLights.point.data(),
        m_packedLights.point.size() * sizeof(GPUPointLight),
        sizeof(GPUPointLight) * LightDefaults::MAX_POINT_LIGHTS
    );

    UpdateBuffer(
        commandList,
        m_spotBuffer.Get(),
        m_spotUpload.Get(),
        m_packedLights.spot.data(),
        m_packedLights.spot.size() * sizeof(GPUSpotLight),
        sizeof(GPUSpotLight) * LightDefaults::MAX_SPOT_LIGHTS
    );

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = {
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(),
        app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap()
    };

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootConstantBufferView(1, m_sceneDataCBAddress);
    commandList->SetGraphicsRootConstantBufferView(3, m_lightsAddress);

    commandList->SetGraphicsRootDescriptorTable(
        8,
        app->getModuleDescriptors()
        ->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
        .getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP)
    );

    commandList->SetGraphicsRootDescriptorTable(9, m_directionalHandle);
    commandList->SetGraphicsRootDescriptorTable(10, m_pointHandle);
    commandList->SetGraphicsRootDescriptorTable(11, m_spotHandle);

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

    result.directional.reserve(LightDefaults::MAX_DIRECTIONAL_LIGHTS);
    result.point.reserve(LightDefaults::MAX_POINT_LIGHTS);
    result.spot.reserve(LightDefaults::MAX_SPOT_LIGHTS);

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
void MeshRendererPass::CreateBuffer(ComPtr<ID3D12Resource>& defaultBuffer, ComPtr<ID3D12Resource>& uploadBuffer, size_t size)
{
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    m_device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&defaultBuffer)
    );

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);

    m_device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );
}

void MeshRendererPass::UpdateBuffer( ID3D12GraphicsCommandList* cmd, ID3D12Resource* dst, ID3D12Resource* upload, const void* data, size_t dataSize, size_t maxSize)
{
    void* mapped;
    upload->Map(0, nullptr, &mapped);

    memset(mapped, 0, maxSize);

    if (data && dataSize > 0)
        memcpy(mapped, data, std::min(dataSize, maxSize));

    upload->Unmap(0, nullptr);

    CD3DX12_RESOURCE_BARRIER toCopy =
        CD3DX12_RESOURCE_BARRIER::Transition(
            dst,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            D3D12_RESOURCE_STATE_COPY_DEST);

    cmd->ResourceBarrier(1, &toCopy);

    cmd->CopyBufferRegion(dst, 0, upload, 0, maxSize);

    CD3DX12_RESOURCE_BARRIER toRead =
        CD3DX12_RESOURCE_BARRIER::Transition(
            dst,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_GENERIC_READ);

    cmd->ResourceBarrier(1, &toRead);
}
#pragma endregion
