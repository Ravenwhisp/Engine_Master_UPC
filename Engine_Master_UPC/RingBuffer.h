#pragma once
#include <queue>
#include <d3d12.h>

#include "Buffer.h"

class ResourcesModule;

struct AllocationInfo
{
    UINT frameIndex;
    size_t offset;
    size_t size;
};

class RingBuffer : public Buffer 
{
public:
    ~RingBuffer();

    D3D12_GPU_VIRTUAL_ADDRESS allocate(const void* data, size_t size, UINT currentFrame);
    void free(UINT lastCompletedFrame);
    void reset();

    size_t getTotalSize() const { return m_totalMemorySize; }

    friend class ResourcesModule;
protected:
    RingBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> buffer, uint32_t sizeInMB);
private:
    std::deque<AllocationInfo> m_allocationQueue;

    uint8_t* m_mappedData = nullptr;
    size_t m_totalMemorySize = 0;
    size_t m_head = 0; //Oldest allocation
    size_t m_tail = 0; // Position of the next allocation
};