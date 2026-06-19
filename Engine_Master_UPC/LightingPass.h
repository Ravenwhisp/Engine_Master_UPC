#pragma once
#include "IRenderPass.h"
#include "Lights.h"

class LightComponent;

class LightingPass : public IRenderPass {
    LightingPass(ComPtr<ID3D12Device4> device);

    virtual void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    GPULightsConstantBuffer packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const;

private:
    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>		m_rootSignature;
    ComPtr<ID3D12PipelineState>		m_pipelineState;
};