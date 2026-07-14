#pragma once
#include "IRenderPass.h"
#include "Vertex.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

#include <vector>
#include <d3d12.h>

class RingBuffer;

using Microsoft::WRL::ComPtr;

class TrailComponent;

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

	RingBuffer* m_ringBuffer;

	std::unique_ptr<VertexBuffer>	m_vertexBuffer;
	std::unique_ptr<IndexBuffer>	m_indexBuffer;

	std::vector<TrailComponent*> m_trailComponent;
	std::vector<VertexTrails> m_vertices;
	std::vector<uint8_t> m_indices;

	const D3D12_VIEWPORT* m_viewport = nullptr;
	const Matrix* m_view = nullptr;
	const Matrix* m_projection = nullptr;
	const Vector3* m_cameraPosition = nullptr;

};

