#pragma once

#include "IRenderPass.h"
#include "Lights.h"
#include "ShadowTypes.h"

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct Submesh;
class RingBuffer;
class MeshRenderer;
class GameObject;
class LightComponent;

struct SceneLightingSettings;
struct SceneDataCB;

namespace DirectX { namespace SimpleMath { struct Vector3; struct Matrix; } }

using Vector3 = DirectX::SimpleMath::Vector3;
using Matrix = DirectX::SimpleMath::Matrix;

class MeshRendererPass : public IRenderPass {
public:

    struct Transforms 
    {
        Matrix mvp;
        Matrix nm;
    };

    constexpr static uint32_t BLOCK_SIZE{ 8 };

    MeshRendererPass(ComPtr<ID3D12Device4> device);

    virtual void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    GPULightsConstantBuffer packLightsForGPU( const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const;

    void renderMesh(ID3D12GraphicsCommandList* commandList);

	int getTriangleCount() const { return m_trianglesCount; }
	int getMeshCount() const { return m_meshCount; }

private:
    std::vector<MeshRenderer*> m_meshRenderers;

    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>		m_rootSignature;
    ComPtr<ID3D12PipelineState>		m_pipelineState;

    D3D12_GPU_VIRTUAL_ADDRESS m_sceneDataCBAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS m_lightsAddress = 0;

    std::unique_ptr<SceneLightingSettings> m_lighting;
    std::unique_ptr<SceneDataCB> m_sceneDataCB;

    const Matrix* m_projection = nullptr;
    const Matrix* m_view = nullptr;
    
    int m_trianglesCount = 0;
	int m_meshCount = 0;

    // ShadowMap
    D3D12_GPU_VIRTUAL_ADDRESS m_shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE m_shadowMapSRV{};
    bool m_hasShadowData = false;

};