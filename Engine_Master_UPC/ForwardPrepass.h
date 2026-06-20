#pragma once

#include "IRenderPass.h"



class MeshRenderer;



class ForwardPrepass : public IRenderPass
{
public:
	ForwardPrepass(ComPtr<ID3D12Device4> device);

	virtual void prepare(const RenderContext& ctx) override;
	void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
	void createRootSignature();
	void createPipelineState();

	ComPtr<ID3D12Device4>		m_device;
	ComPtr<ID3D12RootSignature>	m_rootSignature;
	ComPtr<ID3D12PipelineState>	m_pipelineState;

	std::vector<MeshRenderer*>	m_meshRenderers;

	D3D12_VIEWPORT              m_viewport = {};
	D3D12_RECT                  m_scissorRect = {};

	const Matrix*				m_view = nullptr;
	const Matrix*				m_projection = nullptr;

	D3D12_GPU_VIRTUAL_ADDRESS   m_sceneDataCBAddress = 0;
	RenderSurface*				m_gbufferSurface = nullptr;
};

