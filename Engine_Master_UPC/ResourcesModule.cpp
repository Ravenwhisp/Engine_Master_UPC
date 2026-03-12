#include "Globals.h"
#include "ResourcesModule.h"
#include "D3D12Module.h"
#include "Application.h"
#include "CommandQueue.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "RingBuffer.h"
#include "Texture.h"

#include "AssetsModule.h"
#include "ModelAsset.h"
#include "BasicMesh.h"
#include "BasicMaterial.h"
#include "TextureAsset.h"
#include <DirectXTex.h>

ResourcesModule::~ResourcesModule()
{
	m_deferredResources.clear();
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
	const uint64_t lastCompletedFrame = m_queue->getCompletedFenceValue();

	int i = 0;
	while (i < static_cast<int>(m_deferredResources.size()))
	{
		if (lastCompletedFrame > m_deferredResources[i].frame)
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

bool ResourcesModule::cleanUp()
{
	m_deferredResources.clear();
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

Texture* ResourcesModule::createDepthBuffer(float width, float height)
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

Texture* ResourcesModule::createRenderTexture(float width, float height)
{
	TextureDesc desc{};
	desc.format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	desc.srvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.width = static_cast<uint32_t>(width);
	desc.height = static_cast<uint32_t>(height);
	desc.views = TextureView::SRV | TextureView::RTV;
	desc.initialState = D3D12_RESOURCE_STATE_COMMON;
	desc.hasClearValue = true;
	desc.clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Color(0.0f, 0.2f, 0.4f, 1.0f));
	return new Texture(GenerateUID(), *m_device.Get(), desc);
}

Texture* ResourcesModule::createNullTexture2D()
{
	TextureDesc desc{};
	desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.width = 1;
	desc.height = 1;
	desc.mipLevels = 1;
	desc.views = TextureView::SRV;
	return new Texture(GenerateUID(), *m_device.Get(), desc);
}

Texture* ResourcesModule::createTexture(const TextureAsset& textureAsset)
{
	TextureDesc desc{};
	desc.format = DirectX::MakeSRGB(textureAsset.getFormat());
	desc.width = static_cast<uint32_t>(textureAsset.getWidth());
	desc.height = static_cast<uint32_t>(textureAsset.getHeight());
	desc.arraySize = static_cast<uint16_t>(textureAsset.getArraySize());
	desc.mipLevels = static_cast<uint16_t>(textureAsset.getMipCount());
	desc.views = TextureView::SRV;
	desc.initialState = D3D12_RESOURCE_STATE_COPY_DEST;

	auto texture = new Texture(textureAsset.getId(), *m_device.Get(), desc);

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

RingBuffer* ResourcesModule::createRingBuffer(size_t size)
{
	size_t totalMemorySize = alignUp(size * (1 << 20), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	ComPtr<ID3D12Resource> buffer = createUploadBuffer(totalMemorySize);
	return new RingBuffer(*m_device.Get(), buffer, totalMemorySize);
}

VertexBuffer* ResourcesModule::createVertexBuffer(const void* data, size_t numVertices, size_t vertexStride)
{
	ComPtr<ID3D12Resource> defaultBuffer = createDefaultBuffer(data, numVertices * vertexStride, "VertexBuffer");
	return new VertexBuffer(*m_device.Get(), defaultBuffer, numVertices, vertexStride);
}

IndexBuffer* ResourcesModule::createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat)
{
	ComPtr<ID3D12Resource> defaultBuffer = createDefaultBuffer(data, numIndices * getSizeByFormat(indexFormat), "IndexBuffer");
	return new IndexBuffer(*m_device.Get(), defaultBuffer, numIndices, indexFormat);
}

void ResourcesModule::deferResourceRelease(ComPtr<ID3D12Resource> resource)
{
	DeferredResource deferred{};
	deferred.frame = m_queue->signal();
	deferred.resource = std::move(resource);
	m_deferredResources.push_back(std::move(deferred));
}

void ResourcesModule::uploadTextureAndTransition(ID3D12Resource* dstTexture, const std::vector<D3D12_SUBRESOURCE_DATA>& subData)
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