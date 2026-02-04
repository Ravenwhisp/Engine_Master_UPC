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
	UINT index{ 0 };

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
	D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return _type; }
	bool HasSpace() const;
	uint32_t FreeSpace() const;

	DescriptorHandle Allocate();
	void Free(UINT handle);
	void ReleaseStaleDescriptors(uint64_t frameNumber);

	ID3D12DescriptorHeap* GetHeap() const { return _heap.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT handle) const {
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(_heap->GetCPUDescriptorHandleForHeapStart(), Handle(handle).index, _descriptorSize);
	}
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT handle) const {
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(_heap->GetGPUDescriptorHandleForHeapStart(), Handle(handle).index, _descriptorSize);
	}

	bool IsShaderVisible() const {
		return _type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	}

private:
	bool ValidHandle(UINT index, UINT genNumber);
	std::vector<Handle> _handles{};
	UINT firstFree = 0;
	UINT genNumber = 0;

	ComPtr<ID3D12DescriptorHeap> _heap;
	D3D12_CPU_DESCRIPTOR_HANDLE _cpuStart{};
	D3D12_GPU_DESCRIPTOR_HANDLE _gpuStart{};
	uint32_t _descriptorSize;
	uint32_t _numDescriptors;
	uint32_t _nextFreeIndex = 0;

	D3D12_DESCRIPTOR_HEAP_TYPE _type{};
};

