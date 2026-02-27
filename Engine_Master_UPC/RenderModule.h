#pragma once
#include "Module.h"
#include "DescriptorsModule.h"
#include "Lights.h"

class RingBuffer;
class RenderTexture;
class DepthBuffer;
class GameObject;
class VertexBuffer;
class IndexBuffer;
class Texture;

class RenderModule: public Module
{
public:
	bool init();
	bool postInit();
	void preRender();
	void render();
	bool cleanUp();

	D3D12_GPU_DESCRIPTOR_HANDLE getGPUEditorScreenRT();
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUPlayScreenRT();
	
	D3D12_GPU_VIRTUAL_ADDRESS	allocateInRingBuffer(const void* data, size_t size);

	bool applySkyboxSettings(bool enabled, const char* cubemapPath);
private:

	void transitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
	void renderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);
	void renderEditorScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);
	void renderPlayScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);
	void renderSkybox(ID3D12GraphicsCommandList4* commandList, const Matrix& viewMatrix, Matrix& projectionMatrix);
	void cleanupSkybox();

	//For now let's store the root signature and the pipeline state here
	ComPtr<ID3D12RootSignature>		m_rootSignature;
	ComPtr<ID3D12PipelineState>		m_pipelineState;

	RingBuffer*						m_ringBuffer;
	DescriptorsModule::SampleType	m_sampleType = DescriptorsModule::SampleType::POINT_CLAMP;

	//another root and pipeline for the skybox
	ComPtr<ID3D12RootSignature> m_skyboxRootSignature;
	ComPtr<ID3D12PipelineState> m_skyboxPipelineState;

	VertexBuffer* m_skyboxVertexBuffer = nullptr;
	IndexBuffer* m_skyboxIndexBuffer = nullptr;
	uint32_t      m_skyboxIndexCount = 0;

	std::unique_ptr<Texture> m_skyboxTexture;
	bool m_hasSkybox = false;

	//Scene Editor Offscreen Render Target
	std::unique_ptr<RenderTexture>	m_editorScreenRT{};
	std::unique_ptr<RenderTexture>	m_playScreenRT{};
	std::unique_ptr<DepthBuffer>	m_editorScreenDS{};
	std::unique_ptr<DepthBuffer>	m_playScreenDS{};
	ImVec2							m_size = ImVec2(800, 600);

	D3D12_GPU_VIRTUAL_ADDRESS buildAndUploadLightsCB();
	GPULightsConstantBuffer packLightsForGPU(const std::vector<GameObject*>& objects, const Vector3& ambientColor, float ambientIntensity) const;
};

