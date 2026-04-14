#pragma once

#include "IRenderPass.h"
#include "UICommands.h"

class VertexBuffer;

class UIImagePass : public IRenderPass
{
public:
    explicit UIImagePass(ComPtr<ID3D12Device4> device);
    ~UIImagePass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void renderImages(ID3D12GraphicsCommandList4* commandList);
    Matrix buildImageMVP(const UIImageCommand& command) const;

private:
    struct UIVertex
    {
        Vector2 position;
        Vector2 texCoord;
        Vector4 color;
    };

private:
    const D3D12_VIEWPORT* m_viewport = nullptr;
    const std::vector<UIImageCommand>* m_commands = nullptr;
    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;

    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::unique_ptr<VertexBuffer> m_quadVertexBuffer;
};