#include "Globals.h"
#include "DescriptorHeap.h"
#include "Application.h"
#include "D3D12Module.h"

DescriptorHeap::DescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t numDescriptors): _type(type), _numDescriptors(numDescriptors)
{
	auto device = app->GetD3D12Module()->GetDevice();
	bool isShaderVisible = false;
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
		isShaderVisible = true;
	}
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = numDescriptors;
	heapDesc.Type = type;
	heapDesc.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap));

	_descriptorSize = device->GetDescriptorHandleIncrementSize(_type);
	_cpuStart = _heap->GetCPUDescriptorHandleForHeapStart();
	_gpuStart = isShaderVisible ? _heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

	UINT nextFreeIndex = 0;
	_handles.resize(_numDescriptors);
	for (Handle& handle : _handles) {
		handle.index = ++nextFreeIndex;
		handle.generation = 0;
	}
}

bool DescriptorHeap::HasSpace() const
{
	return firstFree < _numDescriptors;
}

uint32_t DescriptorHeap::FreeSpace() const
{
	return _numDescriptors - firstFree;
}

DescriptorHandle DescriptorHeap::Allocate()
{
	if (HasSpace()) {
		UINT index = firstFree;

		Handle& handle = _handles[index];
		firstFree = handle.index;
		genNumber = (genNumber + 1) % (1 << 8);

		if(index == -1 && genNumber == 0)
		{
			++genNumber;
		}

		handle.index = index;
		handle.generation = genNumber;
		
		DescriptorHandle descriptorHandle;
		descriptorHandle.cpu = GetCPUHandle(handle);
		if (IsShaderVisible()) {
			descriptorHandle.gpu = GetGPUHandle(handle);
		}
		descriptorHandle.index = handle;
		return descriptorHandle;
	} 

	return DescriptorHandle();
}

void DescriptorHeap::Free(UINT handle)
{
	if (!ValidHandle(Handle(handle).index, Handle(handle).generation)) return;
	UINT index = Handle(handle).index;
	Handle& h = _handles[index];
	h.index = firstFree;

	firstFree = index;
}

void DescriptorHeap::ReleaseStaleDescriptors(uint64_t frameNumber)
{
}

bool DescriptorHeap::ValidHandle(UINT index, UINT genNumber)
{
	return index < _handles.size() && _handles[index].index == index && _handles[index].generation == genNumber;
}

