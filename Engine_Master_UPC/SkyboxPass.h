#pragma once
#include "IRenderPass.h"

#include <Texture.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>
#include <Settings.h>
#include <Skybox.h>

class SkyboxSettings;

class SkyBoxPass : public IRenderPass {
public:
    SkyBoxPass(ComPtr<ID3D12Device4> device, SkyboxSettings& settings);

    void apply(ID3D12GraphicsCommandList4* commandList) override;
    void setView(const Matrix& view) { m_view = &view; }
    void setProjection(const Matrix& projection) { m_projection = &projection; }

    void setSettings(const SkyboxSettings& skybox);
private:
    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>		m_rootSignature;
    ComPtr<ID3D12PipelineState>		m_pipelineState;

    SkyBox* m_skyBox = nullptr;

    //Not sure if this belongs here
    mutable const Matrix*         m_projection = nullptr;
    mutable const Matrix*         m_view = nullptr;
};