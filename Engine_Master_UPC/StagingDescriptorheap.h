#pragma once

// -----------------------------------------------------------------------------
// StagingDescriptorHeap
// -----------------------------------------------------------------------------
// A CPU-only (non-shader-visible) descriptor heap used as the write target when
// creating descriptors (SRVs, UAVs) that will later be copied into a
// shader-visible heap via ID3D12Device::CopyDescriptorsSimple.
//
// D3D12 rule: shader-visible heaps are GPU write-only from the CPU — they
// cannot be used as a CopyDescriptors source. Descriptors must first be written
// here, then copied into the shader-visible heap, which is the only way to
// populate it correctly.
//
// This class manages allocation with a simple free-list of individual slots.
// Blocks are not needed here: each texture owns exactly one slot per view type,
// and contiguous layout is only a concern for the shader-visible side.
//
// Usage:
//   StagingDescriptorHandle h = stagingHeap.allocate();
//   device.CreateShaderResourceView(resource, &desc, h.cpu);
//   device.CopyDescriptorsSimple(1, dstCPU, h.cpu, type);   // copy to shader-visible
//   stagingHeap.free(h);
//
class StagingDescriptorHeap
{
public:
    StagingDescriptorHeap(ComPtr<ID3D12Device4> device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t capacity);
    ~StagingDescriptorHeap() = default;

    StagingDescriptorHeap(const StagingDescriptorHeap&) = delete;
    StagingDescriptorHeap& operator=(const StagingDescriptorHeap&) = delete;

    // Allocate one descriptor slot. Returned handle is only CPU-addressable.
    // Asserts if the heap is full.
    D3D12_CPU_DESCRIPTOR_HANDLE allocate();

    // Return a slot obtained from allocate() back to the free pool.
    void free(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    D3D12_DESCRIPTOR_HEAP_TYPE  getType()     const { return m_type; }
    uint32_t                    capacity()    const { return m_capacity; }
    bool                        hasSpace()    const { return m_firstFree < m_capacity; }

private:
    uint32_t indexOf(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

    ComPtr<ID3D12DescriptorHeap>    m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE     m_cpuStart{};
    uint32_t                        m_descriptorSize{ 0 };
    uint32_t                        m_capacity{ 0 };
    D3D12_DESCRIPTOR_HEAP_TYPE      m_type{};

    // Free-list: m_freeList[i] holds the index of the next free slot.
    // m_capacity is the sentinel meaning "end of list".
    std::vector<uint32_t>           m_freeList{};
    uint32_t                        m_firstFree{ 0 };
};