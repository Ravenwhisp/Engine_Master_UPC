#pragma once
#include "DescriptorHandle.h"
#include <Handle.h>
#include "DescriptorHeapBlock.h"

// -----------------------------------------------------------------------------
// DescriptorHeap
// -----------------------------------------------------------------------------
// A convenience wrapper around ID3D12DescriptorHeap that manages descriptor
// allocation in a simple linear fashion. This class is intended to handle both
// CPU-only and shader-visible descriptor heaps.
//
// Notes:
// - DescriptorHandle may evolve into a more robust "DescriptorAllocation" class
//   similar to what is described in: 
//   https://www.3dgep.com/learning-directx-12-3/#DescriptorAllocation_Class
//
class DescriptorHeap
{
public:
    DescriptorHeap(ComPtr<ID3D12Device4> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

    D3D12_DESCRIPTOR_HEAP_TYPE getType()  const { return m_type; }
    uint32_t                   capacity() const { return m_numDescriptors; }

    // Allocate `count` contiguous descriptors. Returns nullptr if no run of
    // that size is available. count must be >= 1.
    DescriptorHeapBlock* allocateBlock(uint32_t count);

    // Return a previously allocated block to the free pool.
    // Merges adjacent free ranges automatically (coalescing).
    void freeBlock(DescriptorHeapBlock* block);

    // Convenience: single descriptor � equivalent to allocateBlock(1)->getHandle(0)
    DescriptorHandle allocate();
    void             free(UINT handle);

    ID3D12DescriptorHeap* getHeap() const { return m_heap.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(uint32_t index) const {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cpuStart, static_cast<INT>(index), m_descriptorSize);
    }
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(uint32_t index) const {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_gpuStart, static_cast<INT>(index), m_descriptorSize);
    }

    bool isShaderVisible() const {
        return m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
            m_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    }

    uint32_t getDescriptorSize() const { return m_descriptorSize; }

private:
    // A node in the free-list: a contiguous range [start, start+count).
    struct FreeRange {
        uint32_t start{ 0 };
        uint32_t count{ 0 };
    };

    // Sorted by start index. Maintained sorted after every alloc/free.
    std::vector<FreeRange> m_freeList{};

    // Pool of block objects � allocated lazily, reused on free.
    // Indexed by their baseIndex so lookup is O(1).
    std::unordered_map<uint32_t, DescriptorHeapBlock> m_blocks{};

    UINT m_genNumber{ 0 };

    ComPtr<ID3D12DescriptorHeap>  m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE   m_cpuStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE   m_gpuStart{};
    uint32_t                      m_descriptorSize{ 0 };
    uint32_t                      m_numDescriptors{ 0 };
    D3D12_DESCRIPTOR_HEAP_TYPE    m_type{};
};
