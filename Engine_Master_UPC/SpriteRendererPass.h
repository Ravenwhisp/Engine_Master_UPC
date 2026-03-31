#pragma once

#include "IRenderPass.h"

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class SpriteRenderer;
class VertexBuffer;

namespace DirectX { namespace SimpleMath { struct Matrix; } }

using Matrix = DirectX::SimpleMath::Matrix;

class SpriteRendererPass : public IRenderPass
{
public:
    SpriteRendererPass(ComPtr<ID3D12Device4> device);

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void renderSprites(ID3D12GraphicsCommandList4* commandList);
    Matrix buildSpriteMVP(SpriteRenderer* sprite) const;

private:
    std::vector<SpriteRenderer*> m_spriteRenderers;

    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::unique_ptr<VertexBuffer> m_quadVertexBuffer;

    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;
};