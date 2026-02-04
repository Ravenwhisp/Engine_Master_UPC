#pragma once
#include "Module.h"
#include "DescriptorHeap.h"
#include "Resources.h"


struct DefferedDescriptor {
	uint64_t frame = 0;
	Handle handle;
};

// -----------------------------------------------------------------------------
// DescriptorsModule
// -----------------------------------------------------------------------------
// Acts as a centralized manager for all descriptor heaps used by the renderer.
//
// This module abstracts the creation and lifetime management of the different
// descriptor heap types required by Direct3D 12:
//
// • RTV(Render Target Views)
// • DSV(Depth Stencil Views)
// • SRV(Shader Resource Views, e.g.textures, buffers)
// • Samplers(Sampler descriptors for filtering / wrapping modes)
class DescriptorsModule: public Module
{
public:
	enum SampleType
	{
		LINEAR_WRAP,
		POINT_WRAP,
		LINEAR_CLAMP,
		POINT_CLAMP,
		COUNT
	};

	~DescriptorsModule();
	bool init() override;
	void preRender() override;
	bool cleanUp() override;

	DescriptorHeap& GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) { return *m_DescriptorHeapMap[type]; }

	void DefferDescriptorRelease(Handle handle);

private:
	void CreateDefaultSamplers();

	std::unordered_map< D3D12_DESCRIPTOR_HEAP_TYPE, DescriptorHeap*> m_DescriptorHeapMap;

	ComPtr<ID3D12Device4> _device{};
	std::vector<DefferedDescriptor> _defferedDescriptors{};
};

