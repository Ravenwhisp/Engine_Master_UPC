#pragma once

class DescriptorHeapBlock;


//
// Represents a pair of CPU/GPU descriptor handles along with the descriptor index
// in the descriptor heap. This acts as a lightweight utility type returned by
// DescriptorHeap::Allocate().
//
struct DescriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
	UINT handle{ 0 };

	const DescriptorHeapBlock* block{ nullptr };

	constexpr bool IsValid() const {
		return cpu.ptr != 0;
	}

	constexpr bool IsShaderVisible() const {
		return gpu.ptr != 0;
	}
};