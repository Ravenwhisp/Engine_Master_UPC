#pragma once

#include "Module.h"
#include "CommandQueue.h"
#include "UID.h"
#include "WeakCache.h"

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
enum class TextureView : uint8_t;
class BasicMaterial;
class BasicMesh;
class MeshAsset;
class MaterialAsset;
class ICacheable;

// Responsible for creation and management of raw GPU resources in D3D12.
// Handles buffers, textures, render targets, depth stencils, and deferred GPU release.
// Owns no asset-level objects — callers own everything returned here.
class ModuleResources : public Module
{
public:
	ModuleResources(ComPtr<ID3D12Device4> device, CommandQueue* queue);
	~ModuleResources();

	bool init()         override;
	void preRender()    override;
	bool cleanUp()      override;

	ComPtr<ID3D12Resource> createUploadBuffer(size_t size);
	ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, size_t size, const char* name);
	ComPtr<ID3D12Resource> createDefaultBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState, const char* name);


	RingBuffer* createRingBuffer(size_t size);
	VertexBuffer* createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride);
	VertexBuffer* createVertexBuffer(ComPtr<ID3D12Resource> existingResource, size_t numVertices, size_t vertexStride);
	IndexBuffer* createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat, const char* name = "IndexBuffer");


	Texture* createDepthBuffer(float width, float height);
	Texture* createRenderTexture(float width, float height);
	Texture* createNullTexture2D();

	Texture* createTextureInternal(const TextureAsset& textureAsset);

	void deferResourceRelease(ComPtr<ID3D12Resource> resource);

	void uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData);


	std::shared_ptr<Texture>		createTexture(const TextureAsset& textureAsset);
	std::shared_ptr<Texture>		createTexture(ComPtr<ID3D12Resource> existingResource, TextureView views, DXGI_FORMAT rtvFormat = DXGI_FORMAT_UNKNOWN);
	std::shared_ptr<BasicMesh>		createMesh(const MeshAsset& meshAsset);
	std::shared_ptr<BasicMaterial>	createMaterial(const MaterialAsset& materialAsset);

private:
	ComPtr<ID3D12Device4>				m_device;
	CommandQueue* m_queue{ nullptr };
	std::vector<DeferredResource>		m_deferredResources;
	WeakCache<UID, ICacheable>			m_resources;
};