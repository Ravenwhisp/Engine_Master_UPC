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

	std::vector<MeshRenderer*>	m_meshRenderer;
};

