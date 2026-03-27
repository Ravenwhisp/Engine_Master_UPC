#include "Globals.h"
#include "ModuleResources.h"
#include "ModuleD3D12.h"
#include "Application.h"
#include "CommandQueue.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "RingBuffer.h"
#include "Texture.h"

#include "ModuleAssets.h"
#include "BasicMesh.h"
#include "BasicMaterial.h"
#include "TextureAsset.h"
#include <DirectXTex.h>
#include "MeshAsset.h"
#include "MaterialAsset.h"
#include "MD5.h"

ModuleResources::ModuleResources(ComPtr<ID3D12Device4> device, CommandQueue* queue)
{
	m_device = device;
	m_queue = queue;
}

ModuleResources::~ModuleResources()
{
	m_deferredResources.clear();
}

bool ModuleResources::init()
{

	return true;
}


void ModuleResources::preRender()
{
	const uint64_t lastCompletedFrame = m_queue->getCompletedFenceValue();

	int i = 0;
	while (i < static_cast<int>(m_deferredResources.size()))
	{
		if (lastCompletedFrame >= m_deferredResources[i].frame)
		{
			m_deferredResources[i] = m_deferredResources.back();
			m_deferredResources.pop_back();
		}
		else
		{
			++i;
		}
	}
}

bool ModuleResources::cleanUp()
{
	m_resources.clear();
	m_deferredResources.clear();
	return true;
}

ComPtr<ID3D12Resource> ModuleResources::createUploadBuffer(size_t size)
{
	ComPtr<ID3D12Resource> buffer;
	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
	return buffer;
}

ComPtr<ID3D12Resource> ModuleResources::createDefaultBuffer(const void* data, size_t size, const char* name)
{
	ComPtr<ID3D12Resource> buffer;
	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	m_device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer));

	ComPtr<ID3D12Resource> uploadBuffer = createUploadBuffer(size);

	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));
	memcpy(pData, data, size);
	uploadBuffer->Unmap(0, nullptr);

	ComPtr<ID3D12GraphicsCommandList4> commandList = m_queue->getCommandList();
	commandList->CopyResource(buffer.Get(), uploadBuffer.Get());
	m_queue->executeCommandList(commandList);
	m_queue->flush();

	buffer->SetName(std::wstring(name, name + strlen(name)).c_str());
	return buffer;
}

Texture* ModuleResources::createDepthBuffer(float width, float height)
{
	TextureDesc desc{};
	desc.format = DXGI_FORMAT_R32_TYPELESS;
	desc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	desc.srvFormat = DXGI_FORMAT_R32_FLOAT;
	desc.width = static_cast<uint32_t>(width);
	desc.height = static_cast<uint32_t>(height);
	desc.views = TextureView::DSV | TextureView::SRV;
	desc.initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	desc.hasClearValue = true;
	desc.clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
	return new Texture(GenerateUID(), *m_device.Get(), desc);
}

Texture* ModuleResources::createRenderTexture(float width, float height)
{
	TextureDesc desc{};
	desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.srvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.width = static_cast<uint32_t>(width);
	desc.height = static_cast<uint32_t>(height);
	desc.views = TextureView::SRV | TextureView::RTV;
	desc.initialState = D3D12_RESOURCE_STATE_COMMON;
	desc.hasClearValue = true;
	desc.clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, Color(0.0f, 0.2f, 0.4f, 1.0f));
	return new Texture(GenerateUID(), *m_device.Get(), desc);
}

Texture* ModuleResources::createNullTexture2D()
{
	TextureDesc desc{};
	desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.width = 1;
	desc.height = 1;
	desc.mipLevels = 1;
	desc.views = TextureView::SRV;
	return new Texture(GenerateUID(), *m_device.Get(), desc);
}

Texture* ModuleResources::createTextureInternal(const TextureAsset& textureAsset, TextureColorSpace colorSpace)
{
	TextureDesc desc{};
	//DXGI_FORMAT baseFormat = textureAsset.getFormat();
	if (colorSpace == TextureColorSpace::SRGB)
	{
		desc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	else
	{
		desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	desc.width = static_cast<uint32_t>(textureAsset.getWidth());
	desc.height = static_cast<uint32_t>(textureAsset.getHeight());
	desc.arraySize = static_cast<uint16_t>(textureAsset.getArraySize());
	desc.mipLevels = static_cast<uint16_t>(textureAsset.getMipCount());
	desc.views = TextureView::SRV;
	desc.initialState = D3D12_RESOURCE_STATE_COPY_DEST;

	auto texture = new Texture(hashToUID(textureAsset.getId()), *m_device.Get(), desc);

	std::vector<D3D12_SUBRESOURCE_DATA> subData;
	subData.reserve(textureAsset.getImageCount());

	for (const auto& subImg : textureAsset.getImages())
	{
		assert(subImg.pixels.data() != nullptr);
		assert(subImg.rowPitch > 0 && subImg.slicePitch > 0);

		D3D12_SUBRESOURCE_DATA data{};
		data.pData = subImg.pixels.data();
		data.RowPitch = subImg.rowPitch;
		data.SlicePitch = subImg.slicePitch;
		subData.push_back(data);
	}

	uploadTextureAndTransition(texture->getD3D12Resource().Get(), subData);
	return texture;
}

RingBuffer* ModuleResources::createRingBuffer(size_t size)
{
	size_t totalMemorySize = alignUp(size * (1 << 20), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	ComPtr<ID3D12Resource> buffer = createUploadBuffer(totalMemorySize);
	return new RingBuffer(*m_device.Get(), buffer, static_cast<uint32_t>(totalMemorySize));
}

VertexBuffer* ModuleResources::createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride)
{
	ComPtr<ID3D12Resource> defaultBuffer = createDefaultBuffer(data, numVertices * vertexStride, "VertexBuffer");
	return new VertexBuffer(*m_device.Get(), defaultBuffer, numVertices, vertexStride);
}

IndexBuffer* ModuleResources::createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat, const char* name )
{
	ComPtr<ID3D12Resource> defaultBuffer = createDefaultBuffer(data, numIndices * getSizeByFormat(indexFormat), name);
	return new IndexBuffer(*m_device.Get(), defaultBuffer, numIndices, indexFormat);
}

void ModuleResources::deferResourceRelease(ComPtr<ID3D12Resource> resource)
{
	DeferredResource deferred{};
	deferred.frame = m_queue->signal();
	deferred.resource = std::move(resource);
	m_deferredResources.push_back(std::move(deferred));
}

void ModuleResources::uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData)
{
	const UINT subCount = static_cast<UINT>(subData.size());

	ComPtr<ID3D12Resource> stagingBuffer = createUploadBuffer(GetRequiredIntermediateSize(dstTexture, 0, subCount));

	ComPtr<ID3D12GraphicsCommandList4> commandList = m_queue->getCommandList();
	UpdateSubresources(commandList.Get(), dstTexture, stagingBuffer.Get(), 0, 0, subCount, subData.data());

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(dstTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &barrier);

	m_queue->executeCommandList(commandList);
	m_queue->flush();
}

std::shared_ptr<Texture> ModuleResources::createTexture(const TextureAsset& textureAsset, TextureColorSpace colorSpace)
{
	const UID uid = hashToUID(textureAsset.getId() + (colorSpace == TextureColorSpace::SRGB ? "_srgb" : "_linear"));

	if (auto cached = m_resources.getAs<Texture>(uid))
	{
		return cached;
	}


	auto texture = std::shared_ptr<Texture>(app->getModuleResources()->createTextureInternal(textureAsset, colorSpace));
	m_resources.insert(uid, texture);
	return texture;
}

std::shared_ptr<Texture> ModuleResources::createTextureSRGB(const TextureAsset& textureAsset)
{
	return createTexture(textureAsset, TextureColorSpace::SRGB);
}

std::shared_ptr<Texture> ModuleResources::createTextureLinear(const TextureAsset& textureAsset)
{
	return createTexture(textureAsset, TextureColorSpace::Linear);
}

std::shared_ptr<Texture> ModuleResources::createTexture(ComPtr<ID3D12Resource> existingResource, TextureView views, DXGI_FORMAT rtvFormat)
{
	assert(existingResource && "existingResource must not be null");

	auto texture = std::make_shared<Texture>(GenerateUID(), *m_device.Get(), existingResource, views, rtvFormat);

	return texture;
}

std::shared_ptr<BasicMesh> ModuleResources::createMesh(const MeshAsset& meshAsset)
{
	const UID uid = hashToUID(meshAsset.getId());

	if (auto cached = m_resources.getAs<BasicMesh>(uid))
	{
		return cached;
	}


	auto mesh = std::make_shared<BasicMesh>(uid, meshAsset);
	m_resources.insert(uid, mesh);
	return mesh;
}

std::shared_ptr<BasicMaterial> ModuleResources::createMaterial(const MaterialAsset& materialAsset)
{
	const UID uid = hashToUID(materialAsset.getId());

	if (auto cached = m_resources.getAs<BasicMaterial>(uid))
	{
		return cached;
	}

	auto material = std::make_shared<BasicMaterial>(uid, materialAsset);
	m_resources.insert(uid, material);
	return material;
}