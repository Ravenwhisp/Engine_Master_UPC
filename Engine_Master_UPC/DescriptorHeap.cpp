#include "Globals.h"
#include "DescriptorHeap.h"
#include "Application.h"
#include "D3D12Module.h"

DescriptorHeap::DescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t numDescriptors): m_type(type), m_numDescriptors(numDescriptors)
{
	auto device = app->getD3D12Module()->getDevice();
	bool isShaderVisible = false;
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
		isShaderVisible = true;
	}
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = numDescriptors;
	heapDesc.Type = type;
	heapDesc.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heap));

	m_descriptorSize = device->GetDescriptorHandleIncrementSize(m_type);
	m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();
	m_gpuStart = isShaderVisible ? m_heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

	UINT nextFreeIndex = 0;
	m_handles.resize(m_numDescriptors);
	for (Handle& handle : m_handles) 
	{
		handle.index = ++nextFreeIndex;
		handle.generation = 0;
	}
}

bool DescriptorHeap::hasSpace() const
{
	return m_firstFree < m_numDescriptors;
}

uint32_t DescriptorHeap::freeSpace() const
{
	return m_numDescriptors - m_firstFree;
}

DescriptorHandle DescriptorHeap::allocate()
{
	if (hasSpace()) 
	{
		UINT index = m_firstFree;

		Handle& handle = m_handles[index];
		m_firstFree = handle.index;
		m_genNumber = (m_genNumber + 1) % (1 << 8);

		if(index == -1 && m_genNumber == 0)
		{
			++m_genNumber;
		}

		handle.index = index;
		handle.generation = m_genNumber;
		
		DescriptorHandle descriptorHandle;
		descriptorHandle.cpu = getCPUHandle(handle);
		if (isShaderVisible()) {
			descriptorHandle.gpu = getGPUHandle(handle);
		}
		descriptorHandle.handle = handle;
		return descriptorHandle;
	} 

	return DescriptorHandle();
}

void DescriptorHeap::free(UINT handle)
{
	if (!validHandle(Handle(handle).index, Handle(handle).generation)) return;
	UINT index = Handle(handle).index;
	Handle& h = m_handles[index];
	h.index = m_firstFree;

	m_firstFree = index;
}

void DescriptorHeap::releaseStaleDescriptors(uint64_t frameNumber)
{
}

bool DescriptorHeap::validHandle(UINT index, UINT genNumber)
{
	return index < m_handles.size() && m_handles[index].index == index && m_handles[index].generation == genNumber;
}

