#pragma once
#include "Module.h"
#include "DescriptorsModule.h"
#include "Lights.h"
#include <IRenderPass.h>
#include <SkyboxPass.h>
#include <MeshRendererPass.h>
#include <DebugDrawPass.h>
#include <ImGuiPass.h>

class Settings;
class RingBuffer;
class RenderTexture;
class DepthBuffer;
class GameObject;
class VertexBuffer;
class IndexBuffer;
class Texture;

class GameViewModule;

class RenderModule: public Module
{
private:
	struct RenderCamera
	{
		Matrix view;
		Matrix projection;
		Vector3 position;
		bool valid = false;
	};

private:
	Settings* m_settings;
	GameViewModule* m_gameViewModule;

	RingBuffer* m_ringBuffer;
	DescriptorsModule::SampleType	m_sampleType = DescriptorsModule::SampleType::POINT_CLAMP;

	//Scene Editor Offscreen Render Target
	std::unique_ptr<RenderTexture>	m_editorScreenRT{};
	std::unique_ptr<RenderTexture>	m_playScreenRT{};
	std::unique_ptr<DepthBuffer>	m_editorScreenDS{};
	std::unique_ptr<DepthBuffer>	m_playScreenDS{};
	ImVec2							m_size = ImVec2(800, 600);


	/// NEEEEEEEEEEEEEW
	SkyBoxPass* m_skyBoxPass = nullptr;
	MeshRendererPass* m_meshRendererPass = nullptr;
	DebugDrawPass* m_debugDrawPass = nullptr;
	ImGuiPass* m_imGuiPass = nullptr;

	std::vector<IRenderPass*> m_renderPasses;

	int m_triangles;

public:
	bool init();
	void preRender();
	void render();
	bool cleanUp();

	D3D12_GPU_DESCRIPTOR_HANDLE getGPUEditorScreenRT();
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUPlayScreenRT();
	
	D3D12_GPU_VIRTUAL_ADDRESS	allocateInRingBuffer(const void* data, size_t size);

	bool applySkyboxSettings(const SkyboxSettings& settings);

	int getTriangles() { return m_triangles; }
private:
#pragma region RENDERS
	void renderScene(ID3D12GraphicsCommandList4* commandList, const RenderCamera& camera, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, bool renderDebug);

	void renderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect);
	void renderEditorScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);
	void renderPlayScene(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);
	void renderGameToBackbuffer(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect);
#pragma endregion

	void renderToTexture(ID3D12GraphicsCommandList4* commandList, RenderTexture* rt, DepthBuffer* ds, std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE)> renderFunc);
	RenderCamera getEditorCamera();
	RenderCamera getGameCamera();

	void transitionResource(ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
};

