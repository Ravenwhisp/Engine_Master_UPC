#include "Globals.h"
#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(ID3D12Device4& device, size_t numIndices, DXGI_FORMAT indexFormat)
    : Buffer(device, CD3DX12_RESOURCE_DESC::Buffer(numIndices * getSizeByFormat(m_IndexFormat)))
    , m_NumIndices(numIndices)
    , m_IndexFormat(indexFormat)
    , m_IndexBufferView{}
{
    createIndexBufferView();
}

IndexBuffer::IndexBuffer(ID3D12Device4& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    size_t numIndices, DXGI_FORMAT indexFormat)
    : Buffer(device, resource)
    , m_NumIndices(numIndices)
    , m_IndexFormat(indexFormat)
    , m_IndexBufferView{}
{
    createIndexBufferView();
}

void IndexBuffer::createIndexBufferView()
{
    UINT bufferSize = m_NumIndices * getSizeByFormat(m_IndexFormat);

    m_IndexBufferView.BufferLocation = m_Resource->GetGPUVirtualAddress();
    m_IndexBufferView.SizeInBytes = bufferSize;
    m_IndexBufferView.Format = m_IndexFormat;
}