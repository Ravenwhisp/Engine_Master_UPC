#pragma once
#include "IRenderPass.h"
#include "Vertex.h"

#include <vector>
#include <d3d12.h>

class VertexBuffer;
class IndexBuffer;

using Microsoft::WRL::ComPtr;

class TrailPass : public IRenderPass
{
public:

	TrailPass(ComPtr<ID3D12Device4> device);
	~TrailPass() override = default;

	virtual void prepare(const RenderContext& ctx) override;
	void apply(ID3D12GraphicsCommandList4* commandList) override;

private:

	ComPtr<ID3D12Device4>           m_device;
	ComPtr<ID3D12RootSignature>		m_rootSignature;
	ComPtr<ID3D12PipelineState>		m_pipelineState;

	std::unique_ptr<VertexBuffer>	m_vertexBuffer;
	std::unique_ptr<IndexBuffer>	m_indexBuffer;

	std::vector<VertexTrails> m_vertices;
	std::vector<uint8_t> m_indices;

};

