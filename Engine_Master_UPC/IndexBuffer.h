#pragma once
#include "Buffer.h"


static int getSizeByFormat(DXGI_FORMAT format) {
    return (format == DXGI_FORMAT_R8_UINT) ? 1 : (format == DXGI_FORMAT_R16_UINT) ? 2 : 4;
}

class ResourcesModule;
class BasicMesh;

class IndexBuffer: public Buffer
{
public:
    IndexBuffer(ID3D12Device4& device, size_t numIndices, DXGI_FORMAT indexFormat);
    IndexBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> resource, size_t numIndices, DXGI_FORMAT indexFormat);
    virtual ~IndexBuffer() = default;

    D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const{ return m_IndexBufferView; }
    size_t                  getNumIndices() const{ return m_NumIndices; }
    DXGI_FORMAT             getIndexFormat() const { return m_IndexFormat; }

    friend class ResourcesModule;
    friend class BasicMesh;

protected:



    void createIndexBufferView();
private:
    size_t      m_NumIndices;
    DXGI_FORMAT m_IndexFormat;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;


};

