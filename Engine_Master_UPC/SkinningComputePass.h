#pragma once
#include "IRenderPass.h"

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class MeshRenderer;

class SkinningComputePass : public IRenderPass
{
public:
    SkinningComputePass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    std::vector<MeshRenderer*> m_meshRenderers;

    ComPtr<ID3D12Device4>       m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
};

