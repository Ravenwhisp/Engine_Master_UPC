#include "Globals.h"
#include "ResourcesModule.h"
#include "D3D12Module.h"
#include "Application.h"
#include "CommandQueue.h"
#include <DirectXTex.h>
#include <iostream>

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "RingBuffer.h"
#include <RenderTexture.h>

ResourcesModule::~ResourcesModule()
{
	m_defferedResources.clear();
}

bool ResourcesModule::init()
{
	m_device = app->getD3D12Module()->getDevice();
	m_queue = app->getD3D12Module()->getCommandQueue();
	return true;
}

bool ResourcesModule::postInit()
{

	return true;
}

void ResourcesModule::preRender()
{
	UINT lastCompletedFrame = app->getD3D12Module()->getLastCompletedFrame();
	for (int i = 0; i < m_defferedResources.size(); ++i)
	{
		if (lastCompletedFrame > m_defferedResources[i].frame)
		{
			m_defferedResources[i] = m_defferedResources.back();
			m_defferedResources.pop_back();
		}
		else
		{
			++i;
		}
	}
}

bool ResourcesModule::cleanUp()
{

	return true;
}

ComPtr<ID3D12Resource> ResourcesModule::createUploadBuffer(size_t size)
{
	ComPtr<ID3D12Resource> buffer;

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));

	return buffer;
}

ComPtr<ID3D12Resource> ResourcesModule::createDefaultBuffer(const void* data, size_t size, const char* name)
{
	// --- CREATE THE FINAL GPU BUFFER (DEFAULT HEAP) ---
	ComPtr<ID3D12Resource> buffer;
	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	m_device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer));
	// --- CREATE THE STAGING BUFFER (UPLOAD HEAP) ---
	ComPtr<ID3D12Resource> uploadBuffer = createUploadBuffer(size);

	// --- CPU: FILL STAGING BUFFER ---
	// Map the buffer: get a CPU pointer to its memory
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0); // We won't read from it, so range is (0,0)
	uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));
	// Copy our application data into the GPU buffer
	memcpy(pData, data, size);
	// Unmap the buffer (invalidate the pointer)
	uploadBuffer->Unmap(0, nullptr);

	// Copy buffer commands

	ComPtr<ID3D12GraphicsCommandList4> _commandList = m_queue->getCommandList();

	_commandList->CopyResource(buffer.Get(), uploadBuffer.Get());

	m_queue->executeCommandList(_commandList);

	m_queue->flush();

	buffer->SetName(std::wstring(name, name + strlen(name)).c_str());
	return buffer;
}

std::unique_ptr<DepthBuffer> ResourcesModule::createDepthBuffer(float windowWidth, float windowHeight)
{
	TextureInitInfo info{};
	info.clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
	info.initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, windowWidth, windowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	info.desc = &desc;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
	if (info.desc->Format == DXGI_FORMAT_D32_FLOAT) {
		srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
	}

	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.PlaneSlice = 0;
	srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

	info.srvDesc = &srv_desc;

	auto buffer = std::make_unique<DepthBuffer>(*m_device.Get(), info);

	return buffer;
}

std::unique_ptr<Texture> ResourcesModule::createTexture2DFromFile(const path& filePath)
{

	ScratchImage image;
	const wchar_t* path = filePath.c_str();

	if (FAILED(LoadFromDDSFile(path, DDS_FLAGS_NONE, nullptr, image)))
	{
		if (FAILED(LoadFromTGAFile(path, nullptr, image)))
		{
			if (FAILED(LoadFromWICFile(path, WIC_FLAGS_NONE, nullptr, image)))
			{
				DEBUG_ERROR("ERROR loading texture, not found valid file.");
				return nullptr;
			}
		}
	}

	if (image.GetImageCount() == 0) {
		return createNullTexture2D();
	}

	TexMetadata metaData = image.GetMetadata();
	if (metaData.dimension != TEX_DIMENSION_TEXTURE2D) {
		return createNullTexture2D();
	}

	generateMipmapsIfMissing(image, metaData);

	TextureInitInfo info{};
	DXGI_FORMAT texFormat = DirectX::MakeSRGB(metaData.format);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(texFormat, UINT64(metaData.width), UINT(metaData.height), UINT16(metaData.arraySize), UINT16(metaData.mipLevels));
	info.desc = &desc;
	info.initialState = D3D12_RESOURCE_STATE_COPY_DEST;
	auto texture = std::make_unique<Texture>(*m_device.Get(), info);

	std::vector<D3D12_SUBRESOURCE_DATA> subData;
	buildSubresourceData(image, metaData, subData);

	uploadTextureAndTransition(texture->getD3D12Resource().Get(), subData);

	return texture;
}

std::unique_ptr<Texture> ResourcesModule::createNullTexture2D()
{
	TextureInitInfo info{};
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
	info.desc = &desc;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Standard format
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	info.srvDesc = &srvDesc;
	auto texture = std::make_unique<Texture>(*m_device.Get(), info);
	return texture;
}

std::unique_ptr<Texture> ResourcesModule::createTextureCubeFromFile(const path& filePath, const char* name)
{
	ScratchImage image;
	const wchar_t* path = filePath.c_str();

	if (FAILED(LoadFromDDSFile(path, DDS_FLAGS_NONE, nullptr, image)))
	{
		return nullptr;
	}

	if (image.GetImageCount() == 0) {
		return nullptr;
	}

	TexMetadata metaData = image.GetMetadata();

	if (metaData.dimension != TEX_DIMENSION_TEXTURE2D || !metaData.IsCubemap() || metaData.arraySize != 6) {
		return nullptr;
	}

	generateMipmapsIfMissing(image, metaData);

	TextureInitInfo info{};

	DXGI_FORMAT texFormat = DirectX::MakeSRGB(metaData.format);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(texFormat, UINT64(metaData.width), UINT(metaData.height), UINT16(metaData.arraySize), UINT16(metaData.mipLevels));

	info.desc = &desc;
	info.initialState = D3D12_RESOURCE_STATE_COPY_DEST;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = (UINT)metaData.mipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	info.srvDesc = &srvDesc;

	auto texture = std::make_unique<Texture>(*m_device.Get(), info);

	std::vector<D3D12_SUBRESOURCE_DATA> subData;
	buildSubresourceData(image, metaData, subData);

	uploadTextureAndTransition(texture->getD3D12Resource().Get(), subData);

	return texture;
}

RingBuffer* ResourcesModule::createRingBuffer(size_t size)
{
	size_t totalMemorySize = alignUp(size * (1 << 20), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	ComPtr<ID3D12Resource> buffer = createUploadBuffer(totalMemorySize);
	return new RingBuffer(*m_device.Get(), buffer, totalMemorySize);
}

std::unique_ptr<RenderTexture> ResourcesModule::createRenderTexture(float windowWidth, float windowHeight)
{
	TextureInitInfo info{};
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT64>(windowWidth), static_cast<UINT>(windowHeight), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, Color(0.0f, 0.2f, 0.4f, 1.0f));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	info.srvDesc = &srvDesc;
	info.clearValue = clearValue;
	info.initialState = D3D12_RESOURCE_STATE_COMMON;
	info.desc = &resourceDesc;

	auto texture = std::make_unique<RenderTexture>(*m_device.Get(), info);	return texture;
}

void ResourcesModule::defferResourceRelease(ComPtr<ID3D12Resource> resource)
{
	DefferedResource defferedResource;
	defferedResource.frame = app->getD3D12Module()->getCurrentFrame();
	defferedResource.resource = resource;
	m_defferedResources.push_back(defferedResource);
}

VertexBuffer* ResourcesModule::createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride)
{
	ComPtr<ID3D12Resource> defaultBuffer = createDefaultBuffer(data, numVertices * vertexStride, "VertexBuffer");
	ID3D12Device4& pDevice = *m_device.Get();
	return new VertexBuffer(pDevice, defaultBuffer, numVertices, vertexStride);
}

IndexBuffer* ResourcesModule::createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat)
{
	ComPtr<ID3D12Resource> defaultBuffer = createDefaultBuffer(data, numIndices * getSizeByFormat(indexFormat), "IndexBuffer");
	ID3D12Device4& pDevice = *m_device.Get();
	return new IndexBuffer(pDevice, defaultBuffer, numIndices, indexFormat);
}

void ResourcesModule::generateMipmapsIfMissing(DirectX::ScratchImage& image, DirectX::TexMetadata& metaData)
{
	if (metaData.mipLevels == 1 && (metaData.width > 1 || metaData.height > 1))
	{
		ScratchImage mipImages;
		if (FAILED(GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), TEX_FILTER_FANT | TEX_FILTER_SEPARATE_ALPHA, 0, mipImages))) {
		}
		else {
			image = std::move(mipImages);
			metaData = image.GetMetadata();
		}
	}
}

void ResourcesModule::buildSubresourceData(const ScratchImage& image, const TexMetadata& metaData, std::vector<D3D12_SUBRESOURCE_DATA>& subData) {
	subData.clear();
	subData.reserve(image.GetImageCount());

	for (size_t item = 0; item < metaData.arraySize; ++item)
	{
		for (size_t level = 0; level < metaData.mipLevels; ++level)
		{
			const Image* subImg = image.GetImage(level, item, 0);
			D3D12_SUBRESOURCE_DATA data = { subImg->pixels, subImg->rowPitch, subImg->slicePitch };
			subData.push_back(data);
		}
	}
}

void ResourcesModule::uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData) {
	const UINT subCount = static_cast<UINT>(subData.size());

	ComPtr<ID3D12Resource> stagingBuffer = createUploadBuffer(GetRequiredIntermediateSize(dstTexture, 0, subCount));

	ComPtr<ID3D12GraphicsCommandList4> commandList = m_queue->getCommandList();
	UpdateSubresources(commandList.Get(), dstTexture, stagingBuffer.Get(), 0, 0, subCount, subData.data());

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(dstTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandList->ResourceBarrier(1, &barrier);
	m_queue->executeCommandList(commandList);
	m_queue->flush();
}

void ResourcesModule::destroyVertexBuffer(VertexBuffer*& vertexBuffer)
{
	if (vertexBuffer)
	{
		delete vertexBuffer;
		vertexBuffer = nullptr;
	}
}
void ResourcesModule::destroyIndexBuffer(IndexBuffer*& indexBuffer)
{
	if (indexBuffer)
	{
		delete indexBuffer;
		indexBuffer = nullptr;
	}
}
