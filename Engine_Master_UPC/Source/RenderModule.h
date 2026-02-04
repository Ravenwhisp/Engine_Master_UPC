#pragma once
#include "Module.h"
#include "DescriptorsModule.h"


namespace Emeika { class Scene; }
class RingBuffer;
class RenderTexture;
class DepthBuffer;

class RenderModule: public Module
{
public:
	bool init();
	bool postInit();
	void preRender();
	void render();
	bool cleanUp();

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUScreenRT();
	Emeika::Scene* GetScene() { return scene; }

	D3D12_GPU_VIRTUAL_ADDRESS AllocateInRingBuffer(const void* data, size_t size);
private:

	void TransitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
	void RenderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);
	void RenderScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);

	//For now let's store the root signature and the pipeline state here
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	RingBuffer* ringBuffer;
	DescriptorsModule::SampleType _sampleType = DescriptorsModule::SampleType::POINT_CLAMP;

	Emeika::Scene* scene;
	//Scene Editor Offscreen Render Target
	std::unique_ptr<RenderTexture> screenRT{};
	std::unique_ptr<DepthBuffer> screenDS{};
	ImVec2 size = ImVec2(800, 600);
};

