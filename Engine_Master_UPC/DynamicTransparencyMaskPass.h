#pragma once

#include "IRenderPass.h"
#include "SimpleMath.h"
#include "DynamicTransparencyRenderData.h"

class GameObject;
class OcclusionTargetComponent;
class RenderSurface;

class DynamicTransparencyMaskPass : public IRenderPass
{
public:
    explicit DynamicTransparencyMaskPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void createRootSignature();
    void createPipelineState();

    bool buildRegionForTarget(OcclusionTargetComponent* target, UINT targetIndex);
    bool projectPoint(const Vector3& point, Vector3& screen) const;

private:
    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    RenderSurface* m_renderSurface = nullptr;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;

    DynamicTransparencyMaskCB m_maskCB{};
    D3D12_GPU_VIRTUAL_ADDRESS m_maskCBAddress = 0;

};