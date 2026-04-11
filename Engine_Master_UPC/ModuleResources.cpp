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
#include "SkyboxParams.h"
#include "SkyBox.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <PlatformHelpers.h>
#include "MD5Fwd.h"


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
	desc.shaderVisibleSRV = true;

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
	desc.shaderVisibleSRV = true;

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

Texture* ModuleResources::createIrradianceInternal(const TextureAsset& textureAsset, const IndexBuffer* indexBuffer, SkyBox* skybox)
{
	TextureDesc desc{};
	
	desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	desc.width = static_cast<uint32_t>(textureAsset.getWidth());
	desc.height = static_cast<uint32_t>(textureAsset.getHeight());
	desc.arraySize = static_cast<uint16_t>(textureAsset.getArraySize());
	desc.mipLevels = static_cast<uint16_t>(textureAsset.getMipCount());
	desc.views = TextureView::RTV;
	desc.initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	auto irradianceTexture = new Texture(hashToUID(textureAsset.getId()), *m_device.Get(), desc);

	ComPtr<ID3D12GraphicsCommandList4> commandList = m_queue->getCommandList();
	
	//ROOT SIGNATURE
	ComPtr<ID3D12RootSignature> rootSignature;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
	CD3DX12_DESCRIPTOR_RANGE srvRange;
	CD3DX12_DESCRIPTOR_RANGE sampRange;

	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

	rootParameters[0].InitAsConstants(sizeof(SkyboxParams) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

	rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	//PIPELINESTATE OBJECT
	ComPtr<ID3D12PipelineState> pso;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ComPtr<ID3DBlob> skyboxIrradianceVertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"SkyboxIrradianceVertexShader.cso", &skyboxIrradianceVertexShaderBlob)); 

	ComPtr<ID3DBlob> skyboxIrradiancePixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"SkyboxIrradiancePixelShader.cso", &skyboxIrradiancePixelShaderBlob)); 

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(skyboxIrradianceVertexShaderBlob.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(skyboxIrradiancePixelShaderBlob.Get());
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc = { 1, 0 };

	DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

	commandList->SetPipelineState(pso.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	
	Matrix proj = Matrix::CreatePerspectiveFieldOfView(PI / 2.0f, 1.0f, 0.1f, 100.0f);

	Vector3 front[] = { Vector3(1,0,0), Vector3(-1,0,0), Vector3(0,1,0), Vector3(0,-1,0), Vector3(0,0,1), Vector3(0,0,-1) };
	Vector3 up[]    = { Vector3(0,1,0), Vector3(0,1,0), Vector3(0,0,-1), Vector3(0,0,1), Vector3(0,1,0), Vector3(0,1,0) };

	ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	BEGIN_EVENT(commandList.Get(), "Irradiance Generation");

	for (size_t i = 0; i < desc.arraySize; i++)
	{
		Matrix view = Matrix::CreateLookAt(Vector3::Zero, front[i], up[i]);
		Matrix viewProjection = view * proj;

		viewProjection = viewProjection.Transpose();

		SkyboxParams params{};
		params.vp = viewProjection;
		params.flipX = i <= 1 ? 0 : 1;
		params.flipZ = i <= 1 ? 1 : 0;

		

		commandList->SetGraphicsRoot32BitConstants(0, sizeof(SkyboxParams) / sizeof(UINT32), &params, 0);
		commandList->SetGraphicsRootDescriptorTable(1, skybox->getTexture()->getSRV().gpu);
		commandList->SetGraphicsRootDescriptorTable(2, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = skybox->getVertexBuffer()->getVertexBufferView();
		D3D12_INDEX_BUFFER_VIEW  indexBufferView = skybox->getIndexBuffer()->getIndexBufferView();

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->IASetIndexBuffer(&indexBufferView);


		UINT subResourceIndex = D3D12CalcSubresource(0, i, 0, desc.mipLevels, desc.arraySize);

		CD3DX12_RESOURCE_BARRIER barrierIn = CD3DX12_RESOURCE_BARRIER::Transition(irradianceTexture->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, subResourceIndex);
		commandList->ResourceBarrier(1, &barrierIn);

		auto handle = irradianceTexture->getContiguousRTV(i).cpu;
		commandList->OMSetRenderTargets(1, &handle, 0, nullptr);

		if (PIXIsAttachedForGpuCapture()) PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);
		commandList->DrawIndexedInstanced(static_cast<UINT>(indexBuffer->getNumIndices()), 1,0,0,0);
		if (PIXIsAttachedForGpuCapture()) PIXEndCapture(TRUE);
		
		CD3DX12_RESOURCE_BARRIER barrierOut = CD3DX12_RESOURCE_BARRIER::Transition(irradianceTexture->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, subResourceIndex);
		commandList->ResourceBarrier(1, &barrierOut);
		
	}

	END_EVENT(commandList.Get());

	m_queue->executeCommandList(commandList);
	m_queue->flush();

	return irradianceTexture;
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

std::shared_ptr<Texture> ModuleResources::createIrradiance(const TextureAsset& textureAsset, const IndexBuffer* indexBuffer, SkyBox* skybox)
{
	const UID uid = hashToUID(textureAsset.getId() + "_irradiance");

	//Temporaly disable this to be able to test the createIrradianceInternal function.
	/*if (auto cached = m_resources.getAs<Texture>(uid))
	{
		return cached;
	}*/
	auto texture = std::shared_ptr<Texture>(app->getModuleResources()->createIrradianceInternal(textureAsset, indexBuffer, skybox));
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