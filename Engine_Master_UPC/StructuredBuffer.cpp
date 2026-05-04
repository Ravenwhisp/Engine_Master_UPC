#include "Globals.h"
#include "StructuredBuffer.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"

#include <d3dx12.h>

StructuredBuffer::StructuredBuffer(ID3D12Device4& device, uint32_t elementSize, uint32_t elementCount, const void* initialData)
	: Buffer(device, CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(elementSize) * static_cast<UINT64>(elementCount)))
	, m_elementSize(elementSize)
	, m_elementCount(elementCount)
	, m_bufferSize(static_cast<UINT64>(elementSize) * static_cast<UINT64>(elementCount))
{
	if (m_bufferSize == 0)
	{
		return;
	}

	CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize);

	DXCall(device.CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	if (initialData)
	{
		CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
		DXCall(device.CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_upload)));

		void* mapped = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		DXCall(m_upload->Map(0, &readRange, &mapped));
		memcpy(mapped, initialData, static_cast<size_t>(m_bufferSize));
		m_upload->Unmap(0, nullptr);
	}

	DescriptorHeap& srvHeap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto handle = srvHeap.allocate();
	m_srvCpu = handle.cpu;
	m_srvGpu = handle.gpu;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_elementCount;
	srvDesc.Buffer.StructureByteStride = m_elementSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device.CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_srvCpu);
}

D3D12_GPU_VIRTUAL_ADDRESS StructuredBuffer::getGPUAddress() const
{
	return m_Resource ? m_Resource->GetGPUVirtualAddress() : 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE StructuredBuffer::getSRV() const
{
	return m_srvCpu;
}

D3D12_GPU_DESCRIPTOR_HANDLE StructuredBuffer::getSRVGPU() const
{
	return m_srvGpu;
}

void StructuredBuffer::update(ID3D12GraphicsCommandList4* commandList, const void* data, uint32_t elementCount)
{
	if (!data || elementCount == 0)
		return;

	uint32_t size = elementCount * m_elementSize;
	if (size > m_bufferSize)
		return;

	ComPtr<ID3D12Resource> upload = app->getModuleResources()->createUploadBuffer(size);

	CD3DX12_RESOURCE_BARRIER toCopy = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST
	);

	commandList->ResourceBarrier(1, &toCopy);

	void* mapped = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	upload->Map(0, &readRange, &mapped);
	memcpy(mapped, data, size);
	upload->Unmap(0, nullptr);

	commandList->CopyBufferRegion(
		m_Resource.Get(),
		0,
		upload.Get(),
		0,
		size
	);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	commandList->ResourceBarrier(1, &barrier);
}