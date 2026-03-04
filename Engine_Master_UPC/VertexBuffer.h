#pragma once
#include "Buffer.h"

class ResourcesModule;
class BasicMeshMesh;;

class VertexBuffer: public Buffer
{
public:
    VertexBuffer(ID3D12Device4& device, size_t numVertices, size_t vertexStride);
    VertexBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> resource, size_t numVertices, size_t vertexStride);    
    virtual ~VertexBuffer();

    D3D12_VERTEX_BUFFER_VIEW    getVertexBufferView() const{ return m_VertexBufferView; }
    size_t                      getNumVertices() const{ return m_NumVertices; }

    friend class ResourcesModule;
    friend class BasicMesh;

protected:
    void createVertexBufferView();
private:

	size_t m_NumVertices;
    size_t  m_VertexStride;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};

