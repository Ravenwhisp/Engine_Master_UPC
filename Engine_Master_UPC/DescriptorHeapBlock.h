#pragma once
class DescriptorHeap;
struct DescriptorHandle;

class DescriptorHeapBlock
{
public:
    DescriptorHeapBlock() = default;
    DescriptorHeapBlock(DescriptorHeap* heap, uint32_t baseIndex, uint32_t size, UINT generation);

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(uint32_t slot) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(uint32_t slot) const;

    // Build a full DescriptorHandle (cpu + gpu + back-pointer) for a given slot.
    DescriptorHandle getHandle(uint32_t slot) const;

    uint32_t baseIndex()  const { return m_baseIndex; }
    uint32_t size()       const { return m_size; }
    UINT     generation() const { return m_generation; }
    bool     isValid()    const { return m_heap != nullptr && m_size > 0; }

private:
    void assertContiguous() const;

    DescriptorHeap* m_heap{ nullptr };
    uint32_t        m_baseIndex{ 0 };
    uint32_t        m_size{ 0 };
    UINT            m_generation{ 0 };
};
