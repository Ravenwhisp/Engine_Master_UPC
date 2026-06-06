#pragma once

#include "IRenderPass.h"
#include "ShadowTypes.h"
#include "Texture.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class ShadowMapPass : public IRenderPass
{
public:
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;

    struct ShadowDrawConstants
    {
        Matrix mvp = Matrix::Identity;
    };

public:
    explicit ShadowMapPass(ComPtr<ID3D12Device4> device);
    ~ShadowMapPass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    const Texture* getShadowMap() const { return m_shadowMap.get(); }
    Texture* getShadowMap() { return m_shadowMap.get(); }

private:
    void createRootSignature();
    void createPipelineState();

private:
    ComPtr<ID3D12Device4> m_device;

    std::unique_ptr<Texture> m_shadowMap;

    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
};