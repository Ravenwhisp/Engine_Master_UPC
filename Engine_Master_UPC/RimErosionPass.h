#pragma once

#include "IRenderPass.h"
#include "Lights.h"
#include "ShadowTypes.h"
#include "AssetReference.h"

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct Submesh;
class RingBuffer;
class MeshRenderer;
class GameObject;
class LightComponent;
class Texture;
class DescriptorHeapBlock;

struct SceneLightingSettings;
struct SceneDataCB;

namespace DirectX { namespace SimpleMath { struct Vector3; struct Matrix; } }

using Vector3 = DirectX::SimpleMath::Vector3;
using Matrix = DirectX::SimpleMath::Matrix;

struct ErosionGPUData
{
    float displacementAmount;
    float rimThreshold;
    float rimSoftness;
    float erosionIntensity;
    float brushScale;
    float brushOffsetX;
    float brushOffsetY;
    float preserveSilhouette;
    float erosionColor[4];
    float debugRimMask;
    float pad0;
    float pad1;
    float pad2;
    float paintColor1[4];
    float paintColor2[4];
    float brushNormalStrength;
    float curvatureScale;
    float toonSharpness;
    float pad3;
};

class RimErosionPass : public IRenderPass
{
public:
    struct Transforms
    {
        Matrix mvp;
        Matrix nm;
    };

    constexpr static uint32_t BLOCK_SIZE{ 8 };

    RimErosionPass(ComPtr<ID3D12Device4> device);
    ~RimErosionPass() override;

    virtual void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    GPULightsConstantBuffer packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const;

    void renderRimErosion(ID3D12GraphicsCommandList* commandList);

private:
    std::vector<MeshRenderer*> m_meshRenderers;

    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>     m_rootSignature;
    ComPtr<ID3D12PipelineState>     m_pipelineState;

    D3D12_GPU_VIRTUAL_ADDRESS m_sceneDataCBAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS m_lightsAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS m_erosionCBAddress = 0;

    D3D12_GPU_VIRTUAL_ADDRESS m_shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE m_shadowMapSRV{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_brushTextureSRV{};
    bool m_hasShadowData = false;

    ErosionGPUData m_erosionData = {};

    std::unique_ptr<SceneLightingSettings> m_lighting;
    std::unique_ptr<SceneDataCB> m_sceneDataCB;

    std::shared_ptr<Texture> m_brushTexture;
    DescriptorHeapBlock* m_brushBlock = nullptr;
    AssetReference m_cachedBrushAssetRef;

    const Matrix* m_projection = nullptr;
    const Matrix* m_view = nullptr;
};
