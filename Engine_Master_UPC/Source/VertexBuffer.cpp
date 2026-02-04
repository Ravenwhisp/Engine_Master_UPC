#include "Globals.h"
#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(ID3D12Device4& device, size_t numVertices, size_t vertexStride)
	: Buffer(device, CD3DX12_RESOURCE_DESC::Buffer(numVertices* vertexStride))
	, m_NumVertices(numVertices)
	, m_VertexStride(vertexStride)
	, m_VertexBufferView{}
{
	CreateVertexBufferView();
}

VertexBuffer::VertexBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> resource, size_t numVertices,
	size_t vertexStride)
	: Buffer(device, resource)
	, m_NumVertices(numVertices)
	, m_VertexStride(vertexStride)
	, m_VertexBufferView{}
{
	CreateVertexBufferView();
}

VertexBuffer::~VertexBuffer() {}

void VertexBuffer::CreateVertexBufferView()
{
	m_VertexBufferView.BufferLocation = GetD3D12Resource()->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = m_VertexStride;
	m_VertexBufferView.SizeInBytes = m_NumVertices * m_VertexStride;
}
