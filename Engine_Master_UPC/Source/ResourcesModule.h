#pragma once

#include "Module.h"
#include "CommandQueue.h"
#include <filesystem>
#include "Resources.h"
#include "Application.h"
#include "DescriptorsModule.h"

using namespace std::filesystem;

struct Vertex
{
	Vector3 position;
	Vector2 texCoord0;
	Vector3 normal = Vector3::UnitZ;
};

struct DefferedResource {
	uint64_t frame = 0;
	ComPtr<ID3D12Resource> resource;
};

//Forward declarations
class VertexBuffer;
class IndexBuffer;
class RingBuffer;

// -----------------------------------------------------------------------------
// ResourcesModule
// -----------------------------------------------------------------------------
// This module centralizes all resource creation
// 
class ResourcesModule : public Module
{
public:
	~ResourcesModule();
	bool init() override;
	bool postInit() override;
	void preRender() override;
	bool cleanUp() override;

	ComPtr<ID3D12Resource> CreateUploadBuffer(size_t size);
	ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* data, size_t size, const char* name);
	std::unique_ptr<DepthBuffer> CreateDepthBuffer(float windowWidth, float windowHeight);
	std::unique_ptr<Texture> CreateTexture2DFromFile(const path& filePath, const char* name);
	std::unique_ptr<Texture> CreateNullTexture2D();
	std::unique_ptr<RenderTexture> CreateRenderTexture(float windowWidth, float windowHeight);
	void DefferResourceRelease(ComPtr<ID3D12Resource> resource);
	RingBuffer* CreateRingBuffer(size_t size);

	// This function are well implemented since this is the only way to create Vertex and Index Buffers
	VertexBuffer* CreateVertexBuffer(const void* data, size_t numVertices, size_t vertexStride);
	IndexBuffer* CreateIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat);
private:
	ComPtr<ID3D12Device4> _device;
	CommandQueue* _queue;
	std::vector<DefferedResource> _defferedResources;
};