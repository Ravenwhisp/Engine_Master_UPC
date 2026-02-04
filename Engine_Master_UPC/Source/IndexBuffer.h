#pragma once
#include "Buffer.h"

static const DXGI_FORMAT IndexFormats[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };

static int GetSizeByFormat(DXGI_FORMAT format) {
    return (format == DXGI_FORMAT_R8_UINT) ? 1 : (format == DXGI_FORMAT_R16_UINT) ? 2 : 4;
}

class ResourcesModule;
namespace Emeika { class Mesh; }

class IndexBuffer: public Buffer
{
public:

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
    {
        return m_IndexBufferView;
    }

    size_t GetNumIndices() const
    {
        return m_NumIndices;
    }

    DXGI_FORMAT GetIndexFormat() const
    {
        return m_IndexFormat;
    }

    friend class ResourcesModule;
    friend class Emeika::Mesh;
protected:
    IndexBuffer(ID3D12Device4& device, size_t numIndices, DXGI_FORMAT indexFormat);
    IndexBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> resource, size_t numIndices, DXGI_FORMAT indexFormat);
    virtual ~IndexBuffer() = default;

    void CreateIndexBufferView();
private:
    size_t      m_NumIndices;
    DXGI_FORMAT m_IndexFormat;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;


};

