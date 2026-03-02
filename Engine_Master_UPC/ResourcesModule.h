#pragma once

#include "Module.h"
#include "CommandQueue.h"
#include <filesystem>
#include "Application.h"
#include "DescriptorsModule.h"
#include <DepthBuffer.h>
#include <RenderTexture.h>
#include <DirectXTex.h>
#include "BasicMaterial.h"
#include "BasicMesh.h"
#include "BoundingBox.h"

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
	std::unique_ptr<DepthBuffer>	createDepthBuffer(float windowWidth, float windowHeight);
	std::shared_ptr<Texture>		createTexture2DFromFile(const path& filePath);
	std::shared_ptr<Texture>		createNullTexture2D();
	std::shared_ptr<Texture>		createTextureCubeFromFile(const path& filePath, const char* name);
	std::unique_ptr<RenderTexture>	createRenderTexture(float windowWidth, float windowHeight);
	RingBuffer*						createRingBuffer(size_t size);
	VertexBuffer*					createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride);
	IndexBuffer*					createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat);

	void							destroyVertexBuffer(VertexBuffer*& vertexBuffer);
	void							destroyIndexBuffer(IndexBuffer*& inderxBuffer);
	void							defferResourceRelease(ComPtr<ID3D12Resource> resource);

	std::weak_ptr<Texture> getLoadedTexture(const std::string& path) const;
	bool isTextureLoaded(const std::string& path) const;
	void markTextureAsLoaded(const std::string& path, std::shared_ptr<Texture> resource);
	void markTextureAsNotLoaded(const std::string& path);

	std::weak_ptr<ModelBinaryData> getLoadedModel(const std::string& path) const;
	bool isModelLoaded(const std::string& path) const;
	void markModelAsLoaded(const std::string& path, std::shared_ptr<ModelBinaryData> resource);
	void markModelAsNotLoaded(const std::string& path);

private:
	ComPtr<ID3D12Device4>			m_device;
	CommandQueue*					m_queue;
	std::vector<DefferedResource>	m_defferedResources;

	void generateMipmapsIfMissing(ScratchImage& image, TexMetadata& metaData);
	void buildSubresourceData(const ScratchImage& image, const TexMetadata& metaData, std::vector<D3D12_SUBRESOURCE_DATA>& subData);
	void uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData);

	std::unordered_map<std::string, std::weak_ptr<Texture>> m_loadedTextures;
	std::unordered_map<std::string, std::weak_ptr<ModelBinaryData>> m_loadedModels;
};