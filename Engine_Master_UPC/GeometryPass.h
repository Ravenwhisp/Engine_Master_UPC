#pragma once
#include "IRenderPass.h"
#include "RenderSurface.h"

class MeshRenderer;
struct RenderContext;

class GeometryPass : public IRenderPass {
public:
    static constexpr UINT GBUFFER_COUNT = 4;

    static constexpr DXGI_FORMAT GBUFFER_FORMATS[GBUFFER_COUNT] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM,         // RT0 diffuse
        DXGI_FORMAT_R32G32B32A32_FLOAT,     // RT1 metallic + roughness --> (R = "metallic"), (G = "perceptualRoughness")
        DXGI_FORMAT_R32G32B32A32_FLOAT,     // RT2 normal
        DXGI_FORMAT_R32G32B32A32_FLOAT,     // RT3 position
    };

    static constexpr RenderSurface::AttachmentPoint kSlots[GBUFFER_COUNT] =
    {
        RenderSurface::GBUFFER_DIFFUSE, RenderSurface::GBUFFER_SPECULAR,
        RenderSurface::GBUFFER_NORMAL, RenderSurface::GBUFFER_POSITION,
    };

    GeometryPass(ComPtr<ID3D12Device4> device);

    virtual void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    RenderSurface* getRenderSurface() { return m_gbufferSurface; }

private:
    void createRootSignature();
    void createPipelineState();

    void transitionGBuffer(ID3D12GraphicsCommandList4* cmdList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) const;

    void transitionAndClearTargets(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle) const;
    void setupPipelineAndHeaps(ID3D12GraphicsCommandList4* commandList) const;
    void renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer) const;

    ComPtr<ID3D12Device4>           m_device;
    ComPtr<ID3D12RootSignature>		m_rootSignature;
    ComPtr<ID3D12PipelineState>		m_pipelineState;

    std::vector<MeshRenderer*>  m_meshRenderers;

    D3D12_VIEWPORT              m_viewport = {};
    D3D12_RECT                  m_scissorRect = {};

    D3D12_GPU_VIRTUAL_ADDRESS   m_sceneDataCBAddress = 0;
    RenderSurface* m_gbufferSurface = nullptr;

    const Matrix* m_view = nullptr;
    const Matrix* m_projection = nullptr;
};