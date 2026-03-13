#pragma once

#include "Module.h"
#include "CommandQueue.h"
#include <filesystem>
#include "Application.h"
#include "ModuleDescriptors.h"
#include <DepthBuffer.h>
#include <RenderTexture.h>
#include <DirectXTex.h>
#include <TextureAsset.h>
#include "BasicMaterial.h"
#include "BasicMesh.h"
#include "BoundingBox.h"
#include <ICacheable.h>

using namespace std::filesystem;



struct DefferedResource {
	uint64_t frame = 0;
	ComPtr<ID3D12Resource> resource;
};


// Conceptually this seems like it belongs to ModelComponent, but this avoids a circular dependency AND also it makes more sense for this to be in the 'Resource' layer, since it's data that actually belongs to the ResourceModule, and ModelComponent just references it.
struct ModelBinaryData {
	// Marking these as unique_ptr makes them be deleted when the object is deleted, instead of just the pointers' values being destroyed, since we're marking this object as the ONLY owner
	std::vector<std::unique_ptr<BasicMesh>> m_meshes = {};
	std::vector<std::unique_ptr<BasicMaterial>> m_materials = {};
	Engine::BoundingBox m_boundingBox;
	bool m_hasBounds;
};

//Forward declarations
class VertexBuffer;
class IndexBuffer;
class RingBuffer;

// -----------------------------------------------------------------------------
// ModuleResources
// -----------------------------------------------------------------------------
// This module centralizes all resource creation
// 
class ModuleResources : public Module
{
public:
	~ModuleResources();

	bool init() override;
	bool postInit() override;
	void preRender() override;
	bool cleanUp() override;

	ComPtr<ID3D12Resource>			createUploadBuffer(size_t size);
	ComPtr<ID3D12Resource>			createDefaultBuffer(const void* data, size_t size, const char* name);
	std::unique_ptr<DepthBuffer>	createDepthBuffer(float windowWidth, float windowHeight);

	std::shared_ptr<Texture>		createTexture2D(const TextureAsset& textureAsset);
	std::shared_ptr<Texture>		createNullTexture2D();
	std::shared_ptr<Texture>		createTextureCubeFromFile(const TextureAsset& textureAsset);

	std::shared_ptr<BasicMesh>		createMesh(const MeshAsset& meshAsset);
	std::shared_ptr<BasicMaterial>	createMaterial(const MaterialAsset& materialAsset);

	std::unique_ptr<RenderTexture>	createRenderTexture(float windowWidth, float windowHeight);
	RingBuffer*						createRingBuffer(size_t size);

	std::unique_ptr<VertexBuffer>	createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride);
	std::unique_ptr<IndexBuffer>	createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat);

	void							defferResourceRelease(ComPtr<ID3D12Resource> resource);
private:
	void generateMipmapsIfMissing(ScratchImage& image, TexMetadata& metaData);
	void buildSubresourceData(const ScratchImage& image, const TexMetadata& metaData, std::vector<D3D12_SUBRESOURCE_DATA>& subData);
	void uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData);

	ComPtr<ID3D12Device4>									m_device;
	CommandQueue*											m_queue;
	std::vector<DefferedResource>							m_defferedResources;
	std::unordered_map<UID, std::weak_ptr<ICacheable>>	m_resources;



	template<typename T>
	std::shared_ptr<T> getResource(UID uid) const
	{
		auto it = m_resources.find(uid);
		if (it == m_resources.end()) return nullptr;
		return std::static_pointer_cast<T>(it->second.lock());
	}

	void registerResource(UID uid, std::shared_ptr<ICacheable> resource)
	{
		m_resources.emplace(uid, std::move(resource));
	}
};