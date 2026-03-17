#include "Globals.h"
#include "DescriptorHeapBlock.h"
#include "DescriptorHeap.h"


DescriptorHeapBlock::DescriptorHeapBlock(DescriptorHeap* heap, uint32_t baseIndex,
    uint32_t size, UINT generation)
    : m_heap(heap), m_baseIndex(baseIndex), m_size(size), m_generation(generation)
{
    assertContiguous();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapBlock::getCPUHandle(uint32_t slot) const
{
    assert(m_heap && "Block is not valid");
    assert(slot < m_size && "Slot index out of block range");
    return m_heap->getCPUHandle(m_baseIndex + slot);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapBlock::getGPUHandle(uint32_t slot) const
{
    assert(m_heap && "Block is not valid");
    assert(m_heap->isShaderVisible() && "Heap is not shader-visible");
    assert(slot < m_size && "Slot index out of block range");
    return m_heap->getGPUHandle(m_baseIndex + slot);
}

DescriptorHandle DescriptorHeapBlock::getHandle(uint32_t slot) const
{
    assert(slot < m_size && "Slot index out of block range");
    DescriptorHandle dh;
    dh.cpu = getCPUHandle(slot);
    dh.gpu = m_heap->isShaderVisible() ? getGPUHandle(slot) : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
    dh.block = this;
    Handle h;
    h.index = static_cast<UINT>(m_baseIndex + slot);
    h.generation = m_generation;
    dh.handle = h;
    return dh;
}

void DescriptorHeapBlock::assertContiguous() const
{
    if (m_size < 2) return; // a single slot is trivially contiguous

    const uint32_t stride = m_heap->getDescriptorSize();
    const SIZE_T   base = m_heap->getCPUHandle(m_baseIndex).ptr;

    for (uint32_t slot = 1; slot < m_size; ++slot)
    {
        const SIZE_T expected = base + static_cast<SIZE_T>(slot) * stride;
        const SIZE_T actual = m_heap->getCPUHandle(m_baseIndex + slot).ptr;

        assert(actual == expected
            && "DescriptorHeapBlock: slots are not contiguous in the heap");
    }
}
