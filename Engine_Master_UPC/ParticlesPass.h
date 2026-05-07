#pragma once

#include "IRenderPass.h"
#include "ParticleCommands.h"

class VertexBuffer;

class ParticlesPass : public IRenderPass
{
public:
    explicit ParticlesPass(ComPtr<ID3D12Device4> device);
    ~ParticlesPass() override = default;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
    void renderImages(ID3D12GraphicsCommandList4* commandList);
    Matrix buildImageWorldMatrix(const ParticleCommand& command) const;
    Matrix buildImageVP();

private:
    struct Vertex
    {
        Vector2 position;
        Vector2 texCoord;
    };

private:
    const D3D12_VIEWPORT* m_viewport = nullptr;
    const std::vector<ParticleEmitterCommand>* m_commands;
    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;

    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    std::unique_ptr<VertexBuffer> m_quadVertexBuffer;
};

