#include "Globals.h"
#include "MeshRendererPass.h"
#include <LightComponent.h>

#include "Application.h"
#include "DescriptorsModule.h"
#include "RenderModule.h"
#include <MeshRenderer.h>
#include "SceneModule.h"
#include "D3D12Module.h"
#include <PlatformHelpers.h>
#include <d3dcompiler.h>

MeshRendererPass::MeshRendererPass(ComPtr<ID3D12Device4> device, RingBuffer* ringBuffer): m_device(device)
{
    m_lighting.ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting.ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    m_ringBuffer = ringBuffer;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[6] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange, sampRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, DescriptorsModule::SampleType::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[4].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[5].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(6, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void MeshRendererPass::apply(ID3D12GraphicsCommandList4* commandList)
{

    // Bind root signature (must be set before any draw calls)
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    //Set input assembler
    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootConstantBufferView(1, m_ringBuffer->allocate(&m_sceneDataCB, sizeof(SceneDataCB), app->getD3D12Module()->getCurrentFrame()));

    const D3D12_GPU_VIRTUAL_ADDRESS lightsAddress = buildAndUploadLightsCB();
    commandList->SetGraphicsRootConstantBufferView(3, lightsAddress);

    commandList->SetGraphicsRootDescriptorTable(5, app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(DescriptorsModule::SampleType::LINEAR_CLAMP));

    renderMesh(commandList);
}


void MeshRendererPass::renderMesh(ID3D12GraphicsCommandList* commandList)
{
    for (const auto& renderer : *m_meshRenderers) {

        Transform* transform = renderer->getTransform();
        Matrix mvp = (transform->getGlobalMatrix() * *m_view * *m_projection).Transpose();
        commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

        const auto& materials = renderer->getMaterials();

        for (const auto& mesh : renderer->getMeshes())
        {
            const auto& submeshes = mesh->getSubmeshes();

            for (const Submesh& submesh : submeshes)
            {
                BasicMaterial* material = renderer->getMaterial(submesh.materialId);

                ModelData modelData{};
                modelData.model = transform->getGlobalMatrix().Transpose();
                modelData.normalMat = transform->getNormalMatrix().Transpose();
                modelData.material = material->getMaterial();

                // The numbers of the Root Parameters Index are hardcoded right now, maybe implement it in a enum
                commandList->SetGraphicsRootConstantBufferView(2, app->getRenderModule()->allocateInRingBuffer(&modelData, sizeof(ModelData)));
                commandList->SetGraphicsRootDescriptorTable(4, material->getTexture()->getSRV().gpu);

                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                D3D12_VERTEX_BUFFER_VIEW vbv = mesh->getVertexBuffer()->getVertexBufferView();
                commandList->IASetVertexBuffers(0, 1, &vbv);

                if (mesh->hasIndexBuffer())
                {
                    D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();
                    commandList->IASetIndexBuffer(&ibv);

                    commandList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.indexStart, 0, 0);
                }
            }
        }
    }
}

D3D12_GPU_VIRTUAL_ADDRESS MeshRendererPass::buildAndUploadLightsCB()
{

    GPULightsConstantBuffer lightsCB = packLightsForGPU(app->getSceneModule()->getAllGameObjects(), m_lighting.ambientColor, m_lighting.ambientIntensity);

    return m_ringBuffer->allocate(&lightsCB, sizeof(GPULightsConstantBuffer), app->getD3D12Module()->getCurrentFrame());
}

GPULightsConstantBuffer MeshRendererPass::packLightsForGPU(const std::vector<GameObject*>& objects, const Vector3& ambientColor, float ambientIntensity) const
{
    GPULightsConstantBuffer cb{};
    cb.ambientColor = ambientColor;
    cb.ambientIntensity = ambientIntensity;

    for (GameObject* gameObject : objects)
    {
        if (!gameObject || !gameObject->GetActive()) continue;

        const LightComponent* lightComponent = gameObject->GetComponentAs<LightComponent>(ComponentType::LIGHT);
        if (!lightComponent) continue;

        const LightData& lightData = lightComponent->getData();
        if (!lightData.common.enabled) continue;

        const Transform* transform = gameObject->GetTransform();
        if (!transform) continue;

        const LightCommon& common = lightData.common;
        const Vector3 position = transform->getPosition();
        Vector3 forward = transform->getForward();
        forward.Normalize();

        switch (lightData.type)
        {
        case LightType::DIRECTIONAL:
            if (cb.directionalCount < LightDefaults::MAX_DIRECTIONAL_LIGHTS)
            {
                auto& gpuLight = cb.directionalLights[cb.directionalCount++];
                gpuLight.direction = forward;
                gpuLight.color = common.color;
                gpuLight.intensity = common.intensity;
            }
            break;

        case LightType::POINT:
            if (cb.pointCount < LightDefaults::MAX_POINT_LIGHTS)
            {
                auto& gpuLight = cb.pointLights[cb.pointCount++];
                gpuLight.position = position;
                gpuLight.radius = lightData.parameters.point.radius;
                gpuLight.color = common.color;
                gpuLight.intensity = common.intensity;
            }
            break;

        case LightType::SPOT:
            if (cb.spotCount < LightDefaults::MAX_SPOT_LIGHTS)
            {
                const SpotLightParameters& sp = lightData.parameters.spot;
                auto& gpuLight = cb.spotLights[cb.spotCount++];
                gpuLight.position = position;
                gpuLight.direction = forward;
                gpuLight.radius = sp.radius;
                gpuLight.color = common.color;
                gpuLight.intensity = common.intensity;
                gpuLight.cosineInnerAngle = std::cos(XMConvertToRadians(sp.innerAngleDegrees));
                gpuLight.cosineOuterAngle = std::cos(XMConvertToRadians(sp.outerAngleDegrees));
            }
            break;

        default: break;
        }
    }

    return cb;
}