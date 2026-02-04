#include "Globals.h"
#include "RingBuffer.h"
#include "Application.h"
#include "ResourcesModule.h"

RingBuffer::RingBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> buffer, uint32_t size) : Buffer(device, buffer), totalMemorySize(size) 
{
    CD3DX12_RANGE readRange(0, 0);
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));
}

RingBuffer::~RingBuffer() {
    CD3DX12_RANGE writtenRange(0, totalMemorySize);
    m_Resource->Unmap(0, &writtenRange);
    mappedData = nullptr;
}

D3D12_GPU_VIRTUAL_ADDRESS RingBuffer::Allocate(const void* srcData, size_t size, UINT currentFrame)
{


    // Align the size
    size = alignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    if (size == 0 || size > totalMemorySize)
        return 0;

    size_t allocationOffset;

    // Check if we need to wrap around
    if (tail >= head) {
        // Case 1: tail >= head (normal case, no wrap yet)
        if (tail + size <= totalMemorySize) {
            // Fits in remaining space at the end
            allocationOffset = tail;
            tail += size;
        }
        else {
            // Need to wrap to beginning
            if (size <= head) {
                // Can wrap if enough space at beginning
                allocationOffset = 0;
                tail = size;
            }
            else {
                // Not enough space
                return 0;
            }
        }
    }
    else {
        // Case 2: tail < head (we've already wrapped)
        if (tail + size <= head) {
            // Fits in space between tail and head
            allocationOffset = tail;
            tail += size;
        }
        else {
            // Not enough space
            return 0;
        }
    }

    memcpy(mappedData + allocationOffset, srcData, size);
    allocationQueue.push({ currentFrame, allocationOffset, size });
    return m_Resource->GetGPUVirtualAddress() + allocationOffset;
}


void RingBuffer::Free(UINT lastCompletedFrame)
{
	while (!allocationQueue.empty() && allocationQueue.front().frameIndex < lastCompletedFrame)
	{
        const AllocationInfo& frontAlloc = allocationQueue.front();

        // Only move head if this is the allocation at the current head position
        if (frontAlloc.offset == head) {
            head += frontAlloc.size;

            // Wrap head if it reaches the end
            if (head >= totalMemorySize) {
                head = 0;
            }
        }

        allocationQueue.pop();
	}

    if (allocationQueue.empty()) {
        head = tail;
    }
}

void RingBuffer::Reset()
{
    head = 0;
    tail = 0;

	std::queue<AllocationInfo> empty;
	std::swap(allocationQueue, empty);
}
