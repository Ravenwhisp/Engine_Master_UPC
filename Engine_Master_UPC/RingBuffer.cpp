#include "Globals.h"
#include "RingBuffer.h"
#include "Application.h"
#include "ModuleResources.h"

RingBuffer::RingBuffer(ID3D12Device4& device, ComPtr<ID3D12Resource> buffer, uint32_t size, size_t alignment) : Buffer(device, buffer), m_totalMemorySize(size), m_alignment (alignment)
{
    CD3DX12_RANGE readRange(0, 0);
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedData));
}

RingBuffer::~RingBuffer() {
    CD3DX12_RANGE writtenRange(0, m_totalMemorySize);
    m_Resource->Unmap(0, &writtenRange);
    m_mappedData = nullptr;
}

D3D12_GPU_VIRTUAL_ADDRESS RingBuffer::allocate(const void* srcData, size_t size)
{
    // Real number of valid bytes the caller provided. Never copy more than
    // this: the aligned memcpy below would otherwise read past srcData into
    // neighbouring heap pages and trigger random 0xC0000005 access violations
    // (e.g. ParticlesPass submits N*88 bytes, which is not a 256-byte multiple).
    const size_t dataSize = size;

    // The GPU needs constant-buffer data 256-byte aligned, so the ring
    // reservation uses the aligned size; the copy uses the real size.
    size = alignUp(size, m_alignment);

    if (size == 0 || size > m_totalMemorySize)
        return 0;

    const size_t availableSize = m_totalMemorySize - m_usedMemorySize;
    if (size > availableSize)
        return 0;

    size_t allocationOffset = 0;
    size_t reservationSize = size;

    // Check if we need to wrap around
    if (m_tail >= m_head) {
        // Case 1: tail >= head (normal case, no wrap yet)
        if (m_tail + size <= m_totalMemorySize) {
            // Fits in remaining space at the end
            allocationOffset = m_tail;
            m_tail = (m_tail + size) % m_totalMemorySize;
        }
        else {
            // Need to wrap to beginning
            const size_t wrapPadding = m_totalMemorySize - m_tail;
            if (size <= m_head && wrapPadding + size <= availableSize) {
                // Can wrap if enough space at beginning
                allocationOffset = 0;
                m_tail = size;
                reservationSize += wrapPadding;
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

    memcpy(m_mappedData + allocationOffset, srcData, dataSize);
    m_usedMemorySize += reservationSize;
    m_allocationQueue.push_back({ 0, allocationOffset, reservationSize });
    ++m_pendingAllocationCount;
    return m_Resource->GetGPUVirtualAddress() + allocationOffset;
}

void RingBuffer::commitPendingAllocations(uint64_t fenceValue)
{
    if (m_pendingAllocationCount == 0)
        return;

    assert(fenceValue != 0);
    assert(m_pendingAllocationCount <= m_allocationQueue.size());

    auto allocation = m_allocationQueue.rbegin();
    for (size_t i = 0; i < m_pendingAllocationCount; ++i, ++allocation)
    {
        allocation->fenceValue = fenceValue;
    }

    m_pendingAllocationCount = 0;
}

void RingBuffer::free(uint64_t completedFenceValue)
{
    while (!m_allocationQueue.empty() &&
        m_allocationQueue.front().fenceValue != 0 &&
        m_allocationQueue.front().fenceValue <= completedFenceValue)
    {
        const AllocationInfo& frontAlloc = m_allocationQueue.front();

        m_head = (m_head + frontAlloc.size) % m_totalMemorySize;
        m_usedMemorySize -= frontAlloc.size;

        m_allocationQueue.pop_front();
    }

    if (m_allocationQueue.empty())
    {
        m_head = 0;
        m_tail = 0;
        m_usedMemorySize = 0;
    }
}

void RingBuffer::reset()
{
    m_head = 0;
    m_tail = 0;
    m_usedMemorySize = 0;
    m_pendingAllocationCount = 0;

	std::deque<AllocationInfo> empty;
	std::swap(m_allocationQueue, empty);
}
