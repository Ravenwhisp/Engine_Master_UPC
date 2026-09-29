#pragma once

#include "IRenderPass.h"

#include <array>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Texture;
struct SceneDataCB;

class OutlinePass : public IRenderPass
{
public:
    explicit OutlinePass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void createRootSignature();
    void createPipelineState();

    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};

    const Matrix* m_projection = nullptr;
    const Matrix* m_view = nullptr;

    std::unique_ptr<SceneDataCB> m_sceneDataCB;
    D3D12_GPU_VIRTUAL_ADDRESS m_sceneDataCBAddress = 0;

    Texture* m_outputTexture = nullptr;
    Texture* m_depthTexture = nullptr;
    Texture* m_normalTexture = nullptr;

    D3D12_GPU_VIRTUAL_ADDRESS m_outlineCBAddress = 0;
};