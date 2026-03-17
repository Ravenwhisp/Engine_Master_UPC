#include "Globals.h"
#include "DescriptorHeap.h"
#include "DescriptorHeapBlock.h"
#include "Application.h"
#include "ModuleD3D12.h"

DescriptorHeap::DescriptorHeap(ComPtr<ID3D12Device4> device,
    const D3D12_DESCRIPTOR_HEAP_TYPE type,
    const uint32_t numDescriptors)
    : m_type(type), m_numDescriptors(numDescriptors)
{
    const bool shaderVisible = isShaderVisible();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = type;
    heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heap));

    m_descriptorSize = device->GetDescriptorHandleIncrementSize(m_type);
    m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpuStart = shaderVisible ? m_heap->GetGPUDescriptorHandleForHeapStart()
        : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

    // Start with one big free range covering the entire heap.
    m_freeList.push_back({ 0, m_numDescriptors });
}

DescriptorHeapBlock* DescriptorHeap::allocateBlock(uint32_t count)
{
    assert(count >= 1 && "allocateBlock: count must be >= 1");

    // First-fit search through the sorted free list.
    for (auto it = m_freeList.begin(); it != m_freeList.end(); ++it)
    {
        if (it->count < count) continue;

        const uint32_t start = it->start;

        // Shrink the free range (or remove it entirely).
        if (it->count == count)
            m_freeList.erase(it);
        else
        {
            it->start += count;
            it->count -= count;
        }

        // Bump generation (wraps at 255, skip 0).
        m_genNumber = (m_genNumber + 1) & 0xFF;
        if (m_genNumber == 0) ++m_genNumber;

        // Construct the block in the pool (keyed by baseIndex).
        m_blocks[start] = DescriptorHeapBlock(this, start, count, m_genNumber);
        return &m_blocks[start];
    }

    assert(false && "DescriptorHeap::allocateBlock — no contiguous range large enough");
    return nullptr;
}

void DescriptorHeap::freeBlock(DescriptorHeapBlock* block)
{
    if (!block || !block->isValid()) return;

    const uint32_t start = block->baseIndex();
    const uint32_t count = block->size();

    // Invalidate the block object.
    m_blocks.erase(start);

    // Insert the freed range back into the sorted free-list and coalesce.
    FreeRange freed{ start, count };

    auto it = std::lower_bound(m_freeList.begin(), m_freeList.end(), freed, [](const FreeRange& a, const FreeRange& b) { return a.start < b.start; });

    it = m_freeList.insert(it, freed);

    // Coalesce with next neighbour.
    auto next = std::next(it);
    if (next != m_freeList.end() && it->start + it->count == next->start)
    {
        it->count += next->count;
        m_freeList.erase(next);
    }

    // Coalesce with previous neighbour.
    if (it != m_freeList.begin())
    {
        auto prev = std::prev(it);
        if (prev->start + prev->count == it->start)
        {
            prev->count += it->count;
            m_freeList.erase(it);
        }
    }
}

DescriptorHandle DescriptorHeap::allocate()
{
    DescriptorHeapBlock* block = allocateBlock(1);
    if (!block) return DescriptorHandle{};
    return block->getHandle(0);
}

void DescriptorHeap::free(UINT handle)
{
    const uint32_t index = Handle(handle).index;
    auto it = m_blocks.find(index);
    if (it != m_blocks.end())
    {
        freeBlock(&it->second);
    }
}


