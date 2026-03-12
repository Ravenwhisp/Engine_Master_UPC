#pragma once

#include "Module.h"
#include "CommandQueue.h"
#include <filesystem>
#include "Application.h"
#include "DescriptorsModule.h"
#include "UID.h"

using namespace std::filesystem;

struct DeferredResource
{
	uint64_t               frame{ 0 };
	ComPtr<ID3D12Resource> resource;
};

class VertexBuffer;
class IndexBuffer;
class RingBuffer;
class BasicMaterial;
class MaterialAsset;
class MeshAsset;
class TextureAsset;
class Texture;
class BasicMesh;
class ICacheable;

class ResourcesModule : public Module
{
public:
	~ResourcesModule();

	bool init()         override;
	bool postInit()     override;
	void preRender()    override;
	bool cleanUp()      override;

	ComPtr<ID3D12Resource>  createUploadBuffer(size_t size);
	ComPtr<ID3D12Resource>  createDefaultBuffer(const void* data, size_t size, const char* name);

	Texture* createTexture2D(const TextureAsset& textureAsset);
	Texture* createTextureCube(const TextureAsset& textureAsset);
	Texture* createNullTexture2D();
	Texture* createRenderTexture(float width, float height);
	Texture* createDepthBuffer(float width, float height);

	BasicMesh* createMesh(const MeshAsset& meshAsset);
	BasicMaterial* createMaterial(const MaterialAsset& materialAsset);

	RingBuffer* createRingBuffer(size_t size);
	VertexBuffer* createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride);
	IndexBuffer* createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat);

	void                    deferResourceRelease(ComPtr<ID3D12Resource> resource);

private:
	Texture* createTextureFromAsset(const TextureAsset& textureAsset);
	void                    uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData);

	ComPtr<ID3D12Device4>                                   m_device;
	CommandQueue* m_queue;
	std::vector<DeferredResource>                           m_deferredResources;
	std::unordered_map<UID, std::shared_ptr<ICacheable>>    m_resources;

	template<typename T>
	T* getResource(UID uid) const
	{
		auto it = m_resources.find(uid);
		if (it == m_resources.end())
		{
			return nullptr;
		}
		return static_cast<T*>(it->second.get());
	}

	void registerResource(UID uid, std::shared_ptr<ICacheable> resource)
	{
		m_resources.emplace(uid, std::move(resource));
	}
};