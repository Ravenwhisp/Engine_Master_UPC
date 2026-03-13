#pragma once

#include "IRenderPass.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class SkyBox;
struct SkyBoxSettings;

namespace DirectX { namespace SimpleMath { struct Matrix; } }

using Matrix = DirectX::SimpleMath::Matrix;

class SkyBoxPass : public IRenderPass {
public:
    SkyBoxPass(ComPtr<ID3D12Device4> device, SkyBoxSettings& settings);
    ~SkyBoxPass();

    void apply(ID3D12GraphicsCommandList4* commandList) override;
    void setView(const Matrix& view) { m_view = &view; }
    void setProjection(const Matrix& projection) { m_projection = &projection; }

    void setSettings(const SkyBoxSettings& settings);
private:
    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>		m_rootSignature;
    ComPtr<ID3D12PipelineState>		m_pipelineState;

    std::unique_ptr<SkyBox>         m_skyBox;

    //Not sure if this belongs here
    const Matrix*         m_projection = nullptr;
    const Matrix*         m_view = nullptr;
};