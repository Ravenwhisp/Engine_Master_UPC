#pragma once
#include <queue>
#include <d3d12.h>

#include "Buffer.h"

class ModuleResources;

struct AllocationInfo
{
    uint64_t fenceValue;
    size_t offset;
    size_t size;
};

class RingBuffer : public Buffer 
{
public:
    ~RingBuffer() override;

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    D3D12_GPU_VIRTUAL_ADDRESS allocate(const void* data, size_t size);
    D3D12_GPU_VIRTUAL_ADDRESS allocate(const void* data, size_t size, uint64_t) { return allocate(data, size); }
    void commitPendingAllocations(uint64_t fenceValue);
    void free(uint64_t completedFenceValue);
    void reset();

    size_t getTotalSize() const { return m_totalMemorySize; }

    friend class ModuleResources;
protected:
    RingBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> buffer, uint32_t sizeInMB, size_t alignment);
private:
    std::deque<AllocationInfo> m_allocationQueue;

    uint8_t* m_mappedData = nullptr;
    size_t m_totalMemorySize = 0;
    size_t m_usedMemorySize = 0;
    size_t m_head = 0; //Oldest allocation
    size_t m_tail = 0; // Position of the next allocation
    size_t m_pendingAllocationCount = 0;

    size_t m_alignment; // for data allocation
};
