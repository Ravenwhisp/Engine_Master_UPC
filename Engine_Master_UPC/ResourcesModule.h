#pragma once

#include "Module.h"
#include "CommandQueue.h"
#include <filesystem>
#include "Application.h"
#include "DescriptorsModule.h"
#include <DepthBuffer.h>
#include <RenderTexture.h>
#include <DirectXTex.h>
#include <TextureAsset.h>

using namespace std::filesystem;



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
	void preRender() override;
	bool cleanUp() override;

	ComPtr<ID3D12Resource>			createUploadBuffer(size_t size);
	ComPtr<ID3D12Resource>			createDefaultBuffer(const void* data, size_t size, const char* name);
	std::unique_ptr<DepthBuffer>	createDepthBuffer(float windowWidth, float windowHeight);

	std::unique_ptr<Texture>		createTexture2DFromFile(const path& filePath, const char* name);
	std::unique_ptr<Texture>		createTexture2D(const TextureAsset& textureAsset);
	std::unique_ptr<Texture>		createNullTexture2D();
	std::unique_ptr<Texture>		createTextureCubeFromFile(const TextureAsset& textureAsset);

	std::unique_ptr<RenderTexture>	createRenderTexture(float windowWidth, float windowHeight);
	RingBuffer*						createRingBuffer(size_t size);

	std::unique_ptr<VertexBuffer>	createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride);
	std::unique_ptr<IndexBuffer>	createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat);
	VertexBuffer*					createVertexBufferPointer(const void* data, size_t numVertices, size_t vertexStride);
	IndexBuffer*					createIndexBufferPointer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat);

	void							destroyVertexBuffer(VertexBuffer*& vertexBuffer);
	void							destroyIndexBuffer(IndexBuffer*& inderxBuffer);
	void							defferResourceRelease(ComPtr<ID3D12Resource> resource);
private:
	ComPtr<ID3D12Device4>			m_device;
	CommandQueue*					m_queue;
	std::vector<DefferedResource>	m_defferedResources;

	void generateMipmapsIfMissing(ScratchImage& image, TexMetadata& metaData);
	void buildSubresourceData(const ScratchImage& image, const TexMetadata& metaData, std::vector<D3D12_SUBRESOURCE_DATA>& subData);
	void uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData);

};