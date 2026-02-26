#pragma once
#include "Module.h"
#include "DescriptorsModule.h"
#include "Lights.h"
#include <IRenderPass.h>
#include <SkyboxPass.h>
#include <MeshRendererPass.h>
#include <DebugDrawPass.h>
#include <ImGuiPass.h>
#include "FontPass.h"

class RingBuffer;
class RenderTexture;
class DepthBuffer;
class GameObject;
class VertexBuffer;
class IndexBuffer;
class Texture;
class Settings;

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

	bool applySkyboxSettings(const SkyboxSettings& settings);
private:
	void renderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect);

	void transitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	Settings* m_settings;

	RingBuffer*						m_ringBuffer;
	DescriptorsModule::SampleType	m_sampleType = DescriptorsModule::SampleType::POINT_CLAMP;

	//Scene Editor Offscreen Render Target
	std::unique_ptr<RenderTexture>	m_screenRT{};
	std::unique_ptr<DepthBuffer>	m_screenDS{};
	ImVec2							m_size = ImVec2(800, 600);


	/// NEEEEEEEEEEEEEW
	SkyBoxPass* m_skyBoxPass = nullptr;
	MeshRendererPass* m_meshRendererPass = nullptr;
	DebugDrawPass* m_debugDrawPass = nullptr;
	ImGuiPass* m_imGuiPass = nullptr;
	FontPass* m_fontPass = nullptr;

	std::vector<IRenderPass*> m_renderPasses;
};

