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
class RenderSurface;

struct SceneLightingSettings;
struct SceneDataCB;

namespace DirectX { namespace SimpleMath { struct Vector3; struct Matrix; } }

using Vector3 = DirectX::SimpleMath::Vector3;
using Matrix = DirectX::SimpleMath::Matrix;

class DeferredShadingPass : public IRenderPass {
public:

    struct Transforms 
    {
        Matrix mvp;
        Matrix nm;
    };

    constexpr static uint32_t BLOCK_SIZE{ 8 };

    DeferredShadingPass(ComPtr<ID3D12Device4> device);

    virtual void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    GPULightsConstantBuffer packLightsForGPU( const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const;

	int getTriangleCount() const { return m_trianglesCount; }
	int getMeshCount() const { return m_meshCount; }

private:
    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>		m_rootSignature;
    ComPtr<ID3D12PipelineState>		m_pipelineState;

    D3D12_GPU_VIRTUAL_ADDRESS m_sceneDataCBAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS m_lightsAddress = 0;

    std::unique_ptr<SceneLightingSettings> m_lighting;
    std::unique_ptr<SceneDataCB> m_sceneDataCB;

    const Matrix* m_projection = nullptr;
    const Matrix* m_view = nullptr;

    RenderSurface* m_renderSurface = nullptr;
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT     m_scissorRect = {};
    
    int m_trianglesCount = 0;
	int m_meshCount = 0;

    // ShadowMap
    D3D12_GPU_VIRTUAL_ADDRESS m_shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE m_shadowMapSRV{};
    bool m_hasShadowData = false;

    // SSAO
    D3D12_GPU_DESCRIPTOR_HANDLE m_ssaoSRV{};
    bool m_hasSSAOData = false;

};