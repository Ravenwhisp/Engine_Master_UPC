#include "Globals.h"
#include "ModuleResources.h"
#include "ModuleD3D12.h"
#include "Application.h"
#include "CommandQueue.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "RingBuffer.h"
#include "Texture.h"
#include "RenderSurface.h"

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
	m_enviromentBrdfTexture.reset();
	m_deferredResources.clear();
	return true;
}

ComPtr<ID3D12Resource> ModuleResources::createUploadBuffer(size_t size)
{
	ComPtr<ID3D12Resource> buffer;
	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	DXCall(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer)));
	return buffer;
}

ComPtr<ID3D12Resource> ModuleResources::createDefaultBuffer(const void* data, size_t size, const char* name)
{
	ComPtr<ID3D12Resource> buffer;
	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	DXCall(m_device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer)));

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

	if (name && name[0] != '\0')
	{
		buffer->SetName(std::wstring(name, name + strlen(name)).c_str());
	}
	return buffer;
}

ComPtr<ID3D12Resource> ModuleResources::createDefaultBuffer(size_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState, const char* name)
{
	ComPtr<ID3D12Resource> buffer;

	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);

	DXCall(m_device->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&buffer)));

	if (buffer && name && name[0] != '\0')
	{
		buffer->SetName(std::wstring(name, name + strlen(name)).c_str());
	}

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

RenderSurface* ModuleResources::createRenderSurface(float width, float height)
{
	auto surface = new RenderSurface();

	auto colorTex = std::shared_ptr<Texture>(app->getModuleResources()->createRenderTexture(width, height));
	colorTex->setName(L"RenderSurface_Color");
	auto depthTex = std::shared_ptr<Texture>(app->getModuleResources()->createDepthBuffer(width, height));
	depthTex->setName(L"RenderSurface_Depth");
	surface->attachTexture(RenderSurface::COLOR_0, colorTex);
	surface->attachTexture(RenderSurface::DEPTH_STENCIL, depthTex);

	return surface;
}

std::shared_ptr<Texture> ModuleResources::createNullTexture2D()
{
	if (auto cached = m_resources.getAs<Texture>(NULL_TEXTURE_HASH))
	{
		DEBUG_LOG("[Cache HIT] NullTexture");
		return cached;
	}

	DEBUG_LOG("[Cache MISS] NullTexture - creating new 1x1 texture");
	TextureDesc desc{};
	desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.width = 1;
	desc.height = 1;
	desc.mipLevels = 1;
	desc.views = TextureView::SRV;
	auto texture = std::make_shared<Texture>(GenerateUID(), *m_device.Get(), desc);
	m_resources.insert(NULL_TEXTURE_HASH, texture);
	return texture;
}

Texture* ModuleResources::createTextureInternal(const TextureAsset& textureAsset, bool shaderVisible)
{
	TextureDesc desc{};
	desc.format = textureAsset.getFormat();

	desc.width = static_cast<uint32_t>(textureAsset.getWidth());
	desc.height = static_cast<uint32_t>(textureAsset.getHeight());
	desc.arraySize = static_cast<uint16_t>(textureAsset.getArraySize());
	desc.mipLevels = static_cast<uint16_t>(textureAsset.getMipCount());
	desc.views = TextureView::SRV;
	desc.initialState = D3D12_RESOURCE_STATE_COPY_DEST;
	desc.shaderVisibleSRV = shaderVisible;

	auto texture = new Texture(textureAsset.getUID(), *m_device.Get(), desc);

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

Texture* ModuleResources::createIrradianceInternal(const IndexBuffer* indexBuffer, SkyBox* skybox)
{
	ComPtr<ID3D12GraphicsCommandList4> commandList = m_queue->getCommandList();
	
	//Texture to render
	TextureDesc desc{};
	desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.width = static_cast<uint32_t>(skybox->getTexture()->getDesc().width);
	desc.height = static_cast<uint32_t>(skybox->getTexture()->getDesc().height);
	desc.arraySize = static_cast<uint16_t>(skybox->getTexture()->getDesc().arraySize);
	desc.mipLevels = 1;
	desc.views = TextureView::RTV;
	desc.initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	auto irradianceTexture = new Texture(GenerateUID(), *m_device.Get(), desc);



	//ROOT SIGNATURE
	ComPtr<ID3D12RootSignature> rootSignature;
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	CD3DX12_ROOT_PARAMETER rootParameters[4] = {};
	CD3DX12_DESCRIPTOR_RANGE srvRange;
	CD3DX12_DESCRIPTOR_RANGE sampRange;
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

	rootParameters[0].InitAsConstants(sizeof(SkyboxParams) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstants(sizeof(TextureSize) / sizeof(UINT32), 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

	rootSignatureDesc.Init(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));



	//PIPELINESTATE OBJECT
	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3DBlob> skyboxIrradianceVertexShaderBlob;
	ComPtr<ID3DBlob> skyboxIrradiancePixelShaderBlob;

	#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
		UINT compileFlags = 0;
	#endif

	ThrowIfFailed(D3DReadFileToBlob(L"SkyboxIrradianceVertexShader.cso", &skyboxIrradianceVertexShaderBlob)); 
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



	//Viewport and scissor
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)desc.width;
	viewport.Height = (float)desc.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor = {};
	scissor.left = 0;
	scissor.top = 0;
	scissor.right = (LONG)desc.width;
	scissor.bottom = (LONG)desc.height;

	TextureSize textureSize = {};
	textureSize.textureSize = viewport.Width;

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissor);
	


	//Descriptors heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);



	//Projection matrix
	Matrix proj = Matrix::CreatePerspectiveFieldOfView(PI / 2.0f, 1.0f, 0.1f, 100.0f);

	Vector3 front[] = { Vector3(1,0,0), Vector3(-1,0,0), Vector3(0,1,0), Vector3(0,-1,0), Vector3(0,0,1), Vector3(0,0,-1) };
	Vector3 up[]    = { Vector3(0,1,0), Vector3(0,1,0), Vector3(0,0,-1), Vector3(0,0,1), Vector3(0,1,0), Vector3(0,1,0) };


	//if (PIXIsAttachedForGpuCapture()) PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);

	for (size_t i = 0; i < desc.arraySize; i++)
	{
		//BEGIN_EVENT(commandList.Get(), "Irradiance Generation");

		Matrix view = Matrix::CreateLookAt(Vector3::Zero, front[i], up[i]);
		Matrix viewProjection = view * proj;
		viewProjection = viewProjection.Transpose();

		SkyboxParams params{};
		params.vp = viewProjection;
		params.flipX = i <= 1 ? 0 : 1;
		params.flipZ = i <= 1 ? 1 : 0;

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = skybox->getVertexBuffer()->getVertexBufferView();
		D3D12_INDEX_BUFFER_VIEW  indexBufferView = skybox->getIndexBuffer()->getIndexBufferView();
		
		UINT subResourceIndex = D3D12CalcSubresource(0, i, 0, desc.mipLevels, desc.arraySize);
		
		auto currentRtvHandle = irradianceTexture->getContiguousRTV(i).cpu;
		
		CD3DX12_RESOURCE_BARRIER barrierIn = CD3DX12_RESOURCE_BARRIER::Transition(irradianceTexture->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, subResourceIndex);
		CD3DX12_RESOURCE_BARRIER barrierOut = CD3DX12_RESOURCE_BARRIER::Transition(irradianceTexture->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, subResourceIndex);
		
		
		
		commandList->SetGraphicsRoot32BitConstants(0, sizeof(SkyboxParams) / sizeof(UINT32), &params, 0);
		commandList->SetGraphicsRoot32BitConstants(1, sizeof(TextureSize) / sizeof(UINT32), &textureSize, 0);
		commandList->SetGraphicsRootDescriptorTable(2, skybox->getTexture()->getSRV().gpu);
		commandList->SetGraphicsRootDescriptorTable(3, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->ResourceBarrier(1, &barrierIn);

		commandList->OMSetRenderTargets(1, &currentRtvHandle, 0, nullptr);
		float clearColor[4] = { 0, 0, 0, 1 };
		commandList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);
		commandList->DrawIndexedInstanced(static_cast<UINT>(indexBuffer->getNumIndices()), 1,0,0,0);

		commandList->ResourceBarrier(1, &barrierOut);
	
		//END_EVENT(commandList.Get());
	}
	
	m_queue->executeCommandList(commandList);
	//if (PIXIsAttachedForGpuCapture()) PIXEndCapture(TRUE);
	
	m_queue->flush();

	auto finalTexture = new Texture(GenerateUID(), *m_device.Get(), irradianceTexture->getD3D12Resource(), TextureView::SRV, DXGI_FORMAT_R16G16B16A16_FLOAT);

	delete irradianceTexture;

	return finalTexture;
}

Texture* ModuleResources::createEnvironmentInternal(const IndexBuffer* indexBuffer, SkyBox* skybox)
{
	ComPtr<ID3D12GraphicsCommandList4> commandList = m_queue->getCommandList();

	//Texture to render
	TextureDesc desc{};
	desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.width = static_cast<uint32_t>(skybox->getTexture()->getDesc().width);
	desc.height = static_cast<uint32_t>(skybox->getTexture()->getDesc().height);
	desc.arraySize = static_cast<uint16_t>(skybox->getTexture()->getDesc().arraySize);
	desc.mipLevels = 11; //roughness levels
	desc.views = TextureView::RTV;
	desc.initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	auto environmentTexture = new Texture(GenerateUID(), *m_device.Get(), desc);


	//ROOT SIGNATURE
	ComPtr<ID3D12RootSignature> rootSignature;
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	CD3DX12_ROOT_PARAMETER rootParameters[5] = {};
	CD3DX12_DESCRIPTOR_RANGE srvRange;
	CD3DX12_DESCRIPTOR_RANGE sampRange;
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

	rootParameters[0].InitAsConstants(sizeof(SkyboxParams) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstants(sizeof(SkyBox::EnvironmentData) / sizeof(UINT32), 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsConstants(sizeof(TextureSize) / sizeof(UINT32), 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[4].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

	rootSignatureDesc.Init(5, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));


	//PIPELINESTATE OBJECT
	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3DBlob> skyboxEnvironmentVertexShaderBlob;
	ComPtr<ID3DBlob> skyboxEnvironmentPixelShaderBlob;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ThrowIfFailed(D3DReadFileToBlob(L"SkyboxEnvironmentVertexShader.cso", &skyboxEnvironmentVertexShaderBlob));
	ThrowIfFailed(D3DReadFileToBlob(L"SkyboxEnvironmentPixelShader.cso", &skyboxEnvironmentPixelShaderBlob));
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(skyboxEnvironmentVertexShaderBlob.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(skyboxEnvironmentPixelShaderBlob.Get());
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






	//Descriptors heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);



	//Projection matrix
	Matrix proj = Matrix::CreatePerspectiveFieldOfView(PI / 2.0f, 1.0f, 0.1f, 100.0f);

	Vector3 front[] = { Vector3(1,0,0), Vector3(-1,0,0), Vector3(0,1,0), Vector3(0,-1,0), Vector3(0,0,1), Vector3(0,0,-1) };
	Vector3 up[] = { Vector3(0,1,0), Vector3(0,1,0), Vector3(0,0,-1), Vector3(0,0,1), Vector3(0,1,0), Vector3(0,1,0) };


	//if (PIXIsAttachedForGpuCapture()) PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);

	for (size_t mip = 0; mip < desc.mipLevels; mip++)
	{
		//Viewport and scissor
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = desc.width >> mip;
		viewport.Height = desc.height >> mip;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissor = {};
		scissor.left = 0;
		scissor.top = 0;
		scissor.right = desc.width >> mip;
		scissor.bottom = desc.height >> mip;

		TextureSize textureSize = {};
		textureSize.textureSize = viewport.Width;

		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissor);

		SkyBox::EnvironmentData environmentData{};
		environmentData.roughness = (float)mip / (desc.mipLevels - 1); //clamp 

		for (size_t i = 0; i < desc.arraySize; i++)
		{
			//BEGIN_EVENT(commandList.Get(), "Environment Generation");

			Matrix view = Matrix::CreateLookAt(Vector3::Zero, front[i], up[i]);
			Matrix viewProjection = view * proj;
			viewProjection = viewProjection.Transpose();

			SkyboxParams params{};
			params.vp = viewProjection;
			params.flipX = i <= 1 ? 0 : 1;
			params.flipZ = i <= 1 ? 1 : 0;


			D3D12_VERTEX_BUFFER_VIEW vertexBufferView = skybox->getVertexBuffer()->getVertexBufferView();
			D3D12_INDEX_BUFFER_VIEW  indexBufferView = skybox->getIndexBuffer()->getIndexBufferView();

			UINT subResourceIndex = D3D12CalcSubresource(mip, i, 0, desc.mipLevels, desc.arraySize);

			auto currentRtvHandle = environmentTexture->getContiguousRTV(mip * desc.arraySize + i).cpu;

			CD3DX12_RESOURCE_BARRIER barrierIn = CD3DX12_RESOURCE_BARRIER::Transition(environmentTexture->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, subResourceIndex);
			CD3DX12_RESOURCE_BARRIER barrierOut = CD3DX12_RESOURCE_BARRIER::Transition(environmentTexture->getD3D12Resource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, subResourceIndex);



			commandList->SetGraphicsRoot32BitConstants(0, sizeof(SkyboxParams) / sizeof(UINT32), &params, 0);
			commandList->SetGraphicsRoot32BitConstants(1, sizeof(SkyBox::EnvironmentData) / sizeof(UINT32), &environmentData, 0);
			commandList->SetGraphicsRoot32BitConstants(2, sizeof(TextureSize) / sizeof(UINT32), &textureSize, 0);
			commandList->SetGraphicsRootDescriptorTable(3, skybox->getTexture()->getSRV().gpu);
			commandList->SetGraphicsRootDescriptorTable(4, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			commandList->IASetIndexBuffer(&indexBufferView);

			commandList->ResourceBarrier(1, &barrierIn);

			commandList->OMSetRenderTargets(1, &currentRtvHandle, 0, nullptr);
			float clearColor[4] = { 0, 0, 0, 1 };
			commandList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);
			commandList->DrawIndexedInstanced(static_cast<UINT>(indexBuffer->getNumIndices()), 1, 0, 0, 0);

			commandList->ResourceBarrier(1, &barrierOut);

			//END_EVENT(commandList.Get());
		}
	}

	m_queue->executeCommandList(commandList);
	//if (PIXIsAttachedForGpuCapture()) PIXEndCapture(TRUE);

	m_queue->flush();




	auto finalTexture = new Texture(GenerateUID(), *m_device.Get(), environmentTexture->getD3D12Resource(), TextureView::SRV, DXGI_FORMAT_R16G16B16A16_FLOAT);

	delete environmentTexture;

	return finalTexture;
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

VertexBuffer* ModuleResources::createVertexBuffer(ComPtr<ID3D12Resource> existingResource, size_t numVertices, size_t vertexStride)
{
	return new VertexBuffer(*m_device.Get(), existingResource, numVertices, vertexStride);
}

IndexBuffer* ModuleResources::createIndexBuffer(const void* data, size_t numIndices, DXGI_FORMAT indexFormat, const char* name )
{
	ComPtr<ID3D12Resource> defaultBuffer = createDefaultBuffer(data, numIndices * getSizeByFormat(indexFormat), name);
	return new IndexBuffer(*m_device.Get(), defaultBuffer, numIndices, indexFormat);
}

void ModuleResources::setEnvironmentBrdfTexture(std::shared_ptr<Texture> texture)
{
	m_enviromentBrdfTexture = texture;
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

std::shared_ptr<Texture> ModuleResources::createTexture(const TextureAsset& textureAsset, bool shaderVisible)
{
	const MD5Hash& libId = textureAsset.getLibId();

	if (auto cached = m_resources.getAs<Texture>(libId))
	{
		return cached;
	}

	auto texture = std::shared_ptr<Texture>(app->getModuleResources()->createTextureInternal(textureAsset, shaderVisible));
	m_resources.insert(libId, texture);
	return texture;
}

std::shared_ptr<Texture> ModuleResources::createIrradiance(const IndexBuffer* indexBuffer, SkyBox* skybox)
{
	auto texture = std::shared_ptr<Texture>(app->getModuleResources()->createIrradianceInternal(indexBuffer, skybox));

	return texture;
}

std::shared_ptr<Texture> ModuleResources::createEnvironment(const IndexBuffer* indexBuffer, SkyBox* skybox)
{
	auto texture = std::shared_ptr<Texture>(app->getModuleResources()->createEnvironmentInternal(indexBuffer, skybox));

	return texture;
}


std::shared_ptr<Texture> ModuleResources::createTexture(ComPtr<ID3D12Resource> existingResource, TextureView views, DXGI_FORMAT rtvFormat)
{
	assert(existingResource && "existingResource must not be null");

	auto texture = std::make_shared<Texture>(GenerateUID(), *m_device.Get(), existingResource, views, rtvFormat);

	return texture;
}

std::shared_ptr<BasicMesh> ModuleResources::createMesh(const MeshAsset& meshAsset)
{
	const MD5Hash& libId = meshAsset.getLibId();

	if (auto cached = m_resources.getAs<BasicMesh>(libId))
	{
		return cached;
	}

	auto mesh = std::make_shared<BasicMesh>(meshAsset.getUID(), meshAsset);
	m_resources.insert(libId, mesh);
	return mesh;
}

std::shared_ptr<BasicMaterial> ModuleResources::createMaterial(MaterialAsset& materialAsset)
{
	const MD5Hash& libId = materialAsset.getLibId();

	if (auto cached = m_resources.getAs<BasicMaterial>(libId))
	{
		return cached;
	}

	auto material = std::make_shared<BasicMaterial>(materialAsset.getUID(), materialAsset);
	m_resources.insert(libId, material);
	return material;
}
