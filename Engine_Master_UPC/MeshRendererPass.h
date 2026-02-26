#pragma once
#include "IRenderPass.h"
#include "Lights.h"
#include "GameObject.h"
#include <RingBuffer.h>
#include <MeshRenderer.h>
#include "SceneModule.h"

class Submesh;

class MeshRendererPass : public IRenderPass {
public:
    MeshRendererPass(ComPtr<ID3D12Device4> device, RingBuffer* ringBuffer);

    void setMeshes(const std::vector<MeshRenderer*>& meshRenderers) { m_meshRenderers = &meshRenderers; }

    void setCameraPosition(const Vector3& cameraPos) { m_sceneDataCB.viewPos = cameraPos; }
    void setView(const Matrix& view) { m_view = &view; }
    void setProjection(const Matrix& projection) { m_projection = &projection; }

    /*void setRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) { m_rtvHandle = rtvHandle; }
    void setDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) { m_dsvHandle = dsvHandle; }

    void setViewport(const D3D12_VIEWPORT& viewport) { m_viewport = &viewport; }
    void setRectScissor(const D3D12_RECT& scissorRect) { m_scissorRect = &scissorRect; }*/


    void apply(ID3D12GraphicsCommandList4* commandList) override;
    D3D12_GPU_VIRTUAL_ADDRESS buildAndUploadLightsCB();
    GPULightsConstantBuffer packLightsForGPU(const std::vector<GameObject*>& objects, const Vector3& ambientColor, float ambientIntensity) const;
    void renderMesh(ID3D12GraphicsCommandList* commandList);

private:
    mutable const std::vector<MeshRenderer*>*     m_meshRenderers;

    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>		m_rootSignature;
    ComPtr<ID3D12PipelineState>		m_pipelineState;

    RingBuffer*                     m_ringBuffer;

    SceneLightingSettings		    m_lighting;
    SceneDataCB					    m_sceneDataCB;

    mutable const Matrix* m_projection;
    mutable const Matrix* m_view;
};