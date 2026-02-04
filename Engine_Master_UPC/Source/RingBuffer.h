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

    D3D12_GPU_VIRTUAL_ADDRESS Allocate(const void* data, size_t size, UINT currentFrame);
    void Free(UINT lastCompletedFrame);
    void Reset();

    size_t GetTotalSize() const { return totalMemorySize; }

    friend class ResourcesModule;
protected:
    RingBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> buffer, uint32_t sizeInMB);
private:
    std::queue<AllocationInfo> allocationQueue;

    uint8_t* mappedData = nullptr;
    size_t totalMemorySize = 0;
    size_t head = 0; //Oldest allocation
    size_t tail = 0; // Position of the next allocation
};