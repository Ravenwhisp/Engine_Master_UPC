#include "Globals.h"
#include "RingBuffer.h"
#include "Application.h"
#include "ResourcesModule.h"

RingBuffer::RingBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> buffer, uint32_t size) : Buffer(device, buffer), m_totalMemorySize(size)
{
    CD3DX12_RANGE readRange(0, 0);
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedData));
}

RingBuffer::~RingBuffer() {
    CD3DX12_RANGE writtenRange(0, m_totalMemorySize);
    m_Resource->Unmap(0, &writtenRange);
    m_mappedData = nullptr;
}

D3D12_GPU_VIRTUAL_ADDRESS RingBuffer::allocate(const void* srcData, size_t size, UINT currentFrame)
{


    // Align the size
    size = alignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    if (size == 0 || size > m_totalMemorySize)
        return 0;

    size_t allocationOffset;

    // Check if we need to wrap around
    if (m_tail >= m_head) {
        // Case 1: tail >= head (normal case, no wrap yet)
        if (m_tail + size <= m_totalMemorySize) {
            // Fits in remaining space at the end
            allocationOffset = m_tail;
            m_tail += size;
        }
        else {
            // Need to wrap to beginning
            if (size <= m_head) {
                // Can wrap if enough space at beginning
                allocationOffset = 0;
                m_tail = size;
            }
            else {
                // Not enough space
                return 0;
            }
        }
    }
    else {
        // Case 2: tail < head (we've already wrapped)
        if (m_tail + size <= m_head) {
            // Fits in space between tail and head
            allocationOffset = m_tail;
            m_tail += size;
        }
        else {
            // Not enough space
            return 0;
        }
    }

    memcpy(m_mappedData + allocationOffset, srcData, size);
    m_allocationQueue.push_back({ currentFrame, allocationOffset, size });
    return m_Resource->GetGPUVirtualAddress() + allocationOffset;
}


void RingBuffer::free(UINT lastCompletedFrame)
{
	while (!m_allocationQueue.empty() && m_allocationQueue.front().frameIndex < lastCompletedFrame)
	{
        const AllocationInfo& frontAlloc = m_allocationQueue.front();

        // Only move head if this is the allocation at the current head position
        if (frontAlloc.offset == m_head) {
            m_head += frontAlloc.size;

            // Wrap head if it reaches the end
            if (m_head >= m_totalMemorySize) {
                m_head = 0;
            }
        }

        m_allocationQueue.pop_front();
	}

    if (m_allocationQueue.empty()) {
        m_head = m_tail;
    }
}

void RingBuffer::reset()
{
    m_head = 0;
    m_tail = 0;

	std::deque<AllocationInfo> empty;
	std::swap(m_allocationQueue, empty);
}
