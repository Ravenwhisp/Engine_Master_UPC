#include "Globals.h"
#include "StagingDescriptorHeap.h"

StagingDescriptorHeap::StagingDescriptorHeap(ComPtr<ID3D12Device4> device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    uint32_t capacity)
    : m_type(type), m_capacity(capacity)
{
    assert(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
        type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV ||
        type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = type;
    desc.NumDescriptors = capacity;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // CPU-only, never shader-visible
    desc.NodeMask = 0;

    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));

    m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
    m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();

    // Build the free-list: slot i points to slot i+1.
    // The last slot points to m_capacity (sentinel = empty).
    m_freeList.resize(capacity);
    for (uint32_t i = 0; i < capacity; ++i)
        m_freeList[i] = i + 1;

    m_firstFree = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE StagingDescriptorHeap::allocate()
{
    assert(hasSpace() && "StagingDescriptorHeap is full");

    const uint32_t index = m_firstFree;
    m_firstFree = m_freeList[index];

    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = m_cpuStart.ptr + static_cast<SIZE_T>(index) * m_descriptorSize;
    return handle;
}

void StagingDescriptorHeap::free(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    const uint32_t index = indexOf(handle);
    assert(index < m_capacity && "Handle does not belong to this heap");

    m_freeList[index] = m_firstFree;
    m_firstFree = index;
}

uint32_t StagingDescriptorHeap::indexOf(D3D12_CPU_DESCRIPTOR_HANDLE handle) const
{
    assert(handle.ptr >= m_cpuStart.ptr);
    return static_cast<uint32_t>((handle.ptr - m_cpuStart.ptr) / m_descriptorSize);
}