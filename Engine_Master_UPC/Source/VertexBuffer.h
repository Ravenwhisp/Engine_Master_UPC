#pragma once
#include "Buffer.h"

class ResourcesModule;
namespace Emeika { class Mesh; };

class VertexBuffer: public Buffer
{
public:
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
    {
        return m_VertexBufferView;
    }

    size_t GetNumVertices() const
    {
        return m_NumVertices;
    }

    friend class ResourcesModule;
    friend class Emeika::Mesh;
protected:
    VertexBuffer(ID3D12Device4& device, size_t numVertices, size_t vertexStride);
    VertexBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> resource, size_t numVertices, size_t vertexStride);
    virtual ~VertexBuffer();

    void CreateVertexBufferView();
private:

	size_t m_NumVertices;
    size_t  m_VertexStride;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};

