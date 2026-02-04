#pragma once


struct Handle {
	UINT index : 24;
	UINT generation : 8;

	Handle() : index(0), generation(0) { ; }
	explicit Handle(UINT handle) { *reinterpret_cast<UINT*>(this) = handle; }
	operator UINT() { return *reinterpret_cast<UINT*>(this); }
};

//
// Represents a pair of CPU/GPU descriptor handles along with the descriptor index
// in the descriptor heap. This acts as a lightweight utility type returned by
// DescriptorHeap::Allocate().
//
struct DescriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
	UINT handle{ 0 };

	constexpr bool IsValid() const {
		return cpu.ptr != 0;
	}

	constexpr bool IsShaderVisible() const {
		return gpu.ptr != 0;
	}
};

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


public:
	DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

	D3D12_DESCRIPTOR_HEAP_TYPE	getType() const { return m_type; }
	bool						hasSpace() const;
	uint32_t					freeSpace() const;

	DescriptorHandle			allocate();
	void						free(UINT handle);
	void						releaseStaleDescriptors(uint64_t frameNumber);

	ID3D12DescriptorHeap*		getHeap() const { return m_heap.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle) const {
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_heap->GetCPUDescriptorHandleForHeapStart(), Handle(handle).index, m_descriptorSize);
	}
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT handle) const {
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_heap->GetGPUDescriptorHandleForHeapStart(), Handle(handle).index, m_descriptorSize);
	}

	bool isShaderVisible() const {
		return m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || m_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	}

private:
	bool validHandle(UINT index, UINT genNumber);

	std::vector<Handle>					m_handles{};
	UINT								m_firstFree = 0;
	UINT								m_genNumber = 0;

	ComPtr<ID3D12DescriptorHeap>		m_heap;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_cpuStart{};
	D3D12_GPU_DESCRIPTOR_HANDLE			m_gpuStart{};
	uint32_t							m_descriptorSize;
	uint32_t							m_numDescriptors;
	uint32_t							m_nextFreeIndex = 0;

	D3D12_DESCRIPTOR_HEAP_TYPE			m_type{};
};

