#pragma once
#include "Module.h"
#include "DescriptorsModule.h"
#include "Lights.h"

class RingBuffer;
class RenderTexture;
class DepthBuffer;
class GameObject;

class RenderModule: public Module
{
public:
	bool init();
	bool postInit();
	void preRender();
	void render();
	bool cleanUp();

	D3D12_GPU_DESCRIPTOR_HANDLE getGPUScreenRT();
	
	D3D12_GPU_VIRTUAL_ADDRESS	allocateInRingBuffer(const void* data, size_t size);
private:

	void transitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
	void renderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);
	void renderScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);

	//For now let's store the root signature and the pipeline state here
	ComPtr<ID3D12RootSignature>		m_rootSignature;
	ComPtr<ID3D12PipelineState>		m_pipelineState;

	RingBuffer*						m_ringBuffer;
	DescriptorsModule::SampleType	m_sampleType = DescriptorsModule::SampleType::POINT_CLAMP;

	//Scene Editor Offscreen Render Target
	std::unique_ptr<RenderTexture>	m_screenRT{};
	std::unique_ptr<DepthBuffer>	m_screenDS{};
	ImVec2							m_size = ImVec2(800, 600);

	D3D12_GPU_VIRTUAL_ADDRESS buildAndUploadLightsCB();
	GPULightsConstantBuffer packLightsForGPU(const std::vector<GameObject*>& objects, const Vector3& ambientColor, float ambientIntensity) const;
};

