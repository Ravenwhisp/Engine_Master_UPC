#pragma once

#include "Module.h"
#include "CommandQueue.h"
#include <filesystem>
#include "Application.h"
#include "DescriptorsModule.h"
#include <DirectXTex.h>
#include "UID.h"

using namespace std::filesystem;


struct DefferedResource {
	uint64_t frame = 0;
	ComPtr<ID3D12Resource> resource;
};

//Forward declarations
class VertexBuffer;
class IndexBuffer;
class RingBuffer;
class BasicMaterial;
class MaterialAsset;
class MeshAsset;
class TextureAsset;
class RenderTexture;
class Texture;
class DepthBuffer;
class BasicMesh;
class ICacheable;

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

	ComPtr<ID3D12Resource>			createUploadBuffer(size_t size);
	ComPtr<ID3D12Resource>			createDefaultBuffer(const void* data, size_t size, const char* name);
	DepthBuffer*					createDepthBuffer(float windowWidth, float windowHeight);

	Texture*						createTexture2D(const TextureAsset& textureAsset);
	Texture*						createNullTexture2D();
	Texture*						createTextureCubeFromFile(const TextureAsset& textureAsset);

	BasicMesh*						createMesh(const MeshAsset& meshAsset);
	BasicMaterial*					createMaterial(const MaterialAsset& materialAsset);

	RenderTexture*					createRenderTexture(float windowWidth, float windowHeight);
	RingBuffer*						createRingBuffer(size_t size);

	VertexBuffer*					createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride);
	IndexBuffer*					createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat);

	void							defferResourceRelease(ComPtr<ID3D12Resource> resource);
private:
	void generateMipmapsIfMissing(ScratchImage& image, TexMetadata& metaData);
	void buildSubresourceData(const ScratchImage& image, const TexMetadata& metaData, std::vector<D3D12_SUBRESOURCE_DATA>& subData);
	void uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData);

	ComPtr<ID3D12Device4>									m_device;
	CommandQueue*											m_queue;
	std::vector<DefferedResource>							m_defferedResources;
	std::unordered_map<UID, std::shared_ptr<ICacheable>>	m_resources;


	template<typename T>
	T* getResource(UID uid) const
	{
		auto it = m_resources.find(uid);
		if (it == m_resources.end()) return nullptr;
		return static_cast<T*>(it->second.get());
	}

	void registerResource(UID uid, std::shared_ptr<ICacheable> resource)
	{
		m_resources.emplace(uid, resource);
	}
};