#pragma once

#include "IRenderPass.h"
#include "Lights.h"

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct Submesh;
class RingBuffer;
class MeshRenderer;
class GameObject;

struct SceneLightingSettings;
struct SceneDataCB;

namespace DirectX { namespace SimpleMath { struct Vector3; struct Matrix; } }

using Vector3 = DirectX::SimpleMath::Vector3;
using Matrix = DirectX::SimpleMath::Matrix;

class MeshRendererPass : public IRenderPass {
public:
    constexpr static uint32_t BLOCK_SIZE{ 8 };

    MeshRendererPass(ComPtr<ID3D12Device4> device, RingBuffer* ringBuffer);
    ~MeshRendererPass();

    void setMeshes(const std::vector<MeshRenderer*>& meshRenderers) { m_meshRenderers = &meshRenderers; }

    void setCameraPosition(const Vector3& cameraPos);
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

    std::unique_ptr<SceneLightingSettings> m_lighting;
    std::unique_ptr<SceneDataCB> m_sceneDataCB;

    const Matrix* m_projection;
    const Matrix* m_view;
};