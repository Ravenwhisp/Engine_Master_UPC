#pragma once

#include "Module.h"
#include "CommandQueue.h"
#include "UID.h"

struct DeferredResource
{
	uint64_t               frame{ 0 };
	ComPtr<ID3D12Resource> resource;
};

class VertexBuffer;
class IndexBuffer;
class RingBuffer;
class Texture;
class TextureAsset;

// Responsible for creation and management of raw GPU resources in D3D12.
// Handles buffers, textures, render targets, depth stencils, and deferred GPU release.
// Owns no asset-level objects — callers own everything returned here.
class ResourcesModule : public Module
{
public:
	ResourcesModule(ComPtr<ID3D12Device4> device, CommandQueue* queue);
	~ResourcesModule();

	bool init()         override;
	void preRender()    override;
	bool cleanUp()      override;

	ComPtr<ID3D12Resource> createUploadBuffer(size_t size);
	ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, size_t size, const char* name);


	RingBuffer* createRingBuffer(size_t size);
	VertexBuffer* createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride);
	IndexBuffer* createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat);


	Texture* createDepthBuffer(float width, float height);
	Texture* createRenderTexture(float width, float height);
	Texture* createNullTexture2D();

	Texture* createTexture(const TextureAsset& textureAsset);

	void deferResourceRelease(ComPtr<ID3D12Resource> resource);

	void uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData);

private:
	ComPtr<ID3D12Device4>         m_device;
	CommandQueue* m_queue{ nullptr };
	std::vector<DeferredResource> m_deferredResources;
};