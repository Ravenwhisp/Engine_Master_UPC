#pragma once

#include "IRenderPass.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class SSAOGeometryPass : public IRenderPass
{
public:
    explicit SSAOGeometryPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    ComPtr<ID3D12Device4> m_device;
};
