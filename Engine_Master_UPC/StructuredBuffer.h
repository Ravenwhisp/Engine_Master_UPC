#pragma once

#include "Buffer.h"

#include <d3d12.h>

class StructuredBuffer : public Buffer
{
public:
	StructuredBuffer(ID3D12Device4& device, uint32_t elementSize, uint32_t elementCount, const void* initialData = nullptr);

	D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getSRV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE getSRVGPU() const;

	uint32_t getElementSize() const { return m_elementSize; }
	uint32_t getElementCount() const { return m_elementCount; }

	ID3D12Resource* getUploadResource() const { return m_upload.Get(); }

	void update(ID3D12GraphicsCommandList4* commandList, const void* data, uint32_t elementCount);

private:
	uint32_t m_elementSize = 0;
	uint32_t m_elementCount = 0;
	uint64_t m_bufferSize = 0;

	ComPtr<ID3D12Resource> m_upload;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpu{};
};
