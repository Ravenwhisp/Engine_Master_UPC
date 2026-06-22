#include "Globals.h"
#include "OutlinePass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "ModuleAssets.h"
#include "RenderContext.h"
#include "Texture.h"
#include "TextureAsset.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include "PlatformHelpers.h"

OutlinePass::OutlinePass(ComPtr<ID3D12Device4> device)
	: m_device(device)
{
	createRootSignature();
	createPipelineState();
}

OutlinePass::~OutlinePass()
{
	releaseManualSRV();
	releaseCopyResources();
	if (m_hasFallbackSRV)
	{
		auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		heap.free(m_fallbackSRV.handle);
		m_fallbackSRV = {};
		m_hasFallbackSRV = false;
	}
	if (m_hasManualNoiseSRV)
	{
		auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		heap.free(m_noiseSRV.handle);
		m_noiseSRV = {};
		m_hasManualNoiseSRV = false;
	}
}

void OutlinePass::releaseManualSRV()
{
	if (m_hasManualSRV && m_manualSRV.IsValid())
	{
		auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		heap.free(m_manualSRV.handle);
		m_manualSRV = {};
		m_hasManualSRV = false;
	}
}

void OutlinePass::releaseCopyResources()
{
	if (m_sceneColorCopySRV.IsValid())
	{
		auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		heap.free(m_sceneColorCopySRV.handle);
		m_sceneColorCopySRV = {};
	}
	if (m_contiguousSRVBlock)
	{
		auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		heap.freeBlock(m_contiguousSRVBlock);
		m_contiguousSRVBlock = nullptr;
		m_srvTableGpu = {};
	}
	m_sceneColorCopy.Reset();
	m_copyWidth = 0;
	m_copyHeight = 0;
}

void OutlinePass::ensureColorCopy(uint32_t width, uint32_t height, DXGI_FORMAT format)
{
	if (m_sceneColorCopy && m_copyWidth == width && m_copyHeight == height)
		return;

	releaseCopyResources();

	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1);
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	DXCall(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_sceneColorCopy)));

	m_sceneColorCopy->SetName(L"OutlineSceneColorCopy");

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_sceneColorCopySRV = heap.allocate();

	m_device->CreateShaderResourceView(m_sceneColorCopy.Get(), &srvDesc, m_sceneColorCopySRV.cpu);

	m_copyWidth = width;
	m_copyHeight = height;
}

void OutlinePass::ensureFallbackTexture()
{
	if (m_fallbackTexture)
		return;

	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	DXCall(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_fallbackTexture)));

	m_fallbackTexture->SetName(L"OutlineFallbackWhite");

	CD3DX12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(65536);
	ComPtr<ID3D12Resource> uploadBuffer;
	DXCall(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)));

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint = {};
	placedFootprint.Offset = 0;
	placedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	placedFootprint.Footprint.Width = 1;
	placedFootprint.Footprint.Height = 1;
	placedFootprint.Footprint.Depth = 1;
	placedFootprint.Footprint.RowPitch = 4;

	uint8_t whitePixel[4] = { 255, 255, 255, 255 };
	void* mapped = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	uploadBuffer->Map(0, &readRange, &mapped);
	memcpy(mapped, whitePixel, 4);
	uploadBuffer->Unmap(0, nullptr);

	ComPtr<ID3D12GraphicsCommandList4> cmdList;
	auto* commandQueue = app->getModuleD3D12()->getCommandQueue();
	cmdList = commandQueue->getCommandList();

	CD3DX12_TEXTURE_COPY_LOCATION dstLoc(m_fallbackTexture.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION srcLoc(uploadBuffer.Get(), placedFootprint);
	cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_fallbackTexture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdList->ResourceBarrier(1, &barrier);

	commandQueue->executeCommandList(cmdList);
	commandQueue->flush();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_fallbackSRV = heap.allocate();
	m_hasFallbackSRV = true;

	m_device->CreateShaderResourceView(m_fallbackTexture.Get(), &srvDesc, m_fallbackSRV.cpu);
}

void OutlinePass::loadNoiseTexture(const AssetReference& assetId)
{
	if (!assetId.isValid())
	{
		if (m_loadedNoiseTexture || m_loadedNoiseAssetId.isValid())
		{
			m_loadedNoiseTexture.reset();
			m_loadedNoiseAssetId = {};
		}
		return;
	}

	if (m_loadedNoiseAssetId == assetId && m_loadedNoiseTexture)
		return;

	m_loadedNoiseTexture.reset();

	auto* moduleAssets = app->getModuleAssets();
	AssetReference ref = assetId;
	auto textureAsset = moduleAssets->load<TextureAsset>(ref);
	if (!textureAsset)
	{
		m_loadedNoiseAssetId = {};
		return;
	}

	m_loadedNoiseTexture = app->getModuleResources()->createTexture(*textureAsset, true);
	m_loadedNoiseAssetId = assetId;
}

void OutlinePass::createRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE srvRange;
	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE samplerRange0;
	samplerRange0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE samplerRange1;
	samplerRange1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[4];
	rootParameters[0].InitAsConstants(28, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &samplerRange0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsDescriptorTable(1, &samplerRange1, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(
		_countof(rootParameters),
		rootParameters,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	DXCall(D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error));

	DXCall(m_device->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)));
}

void OutlinePass::createPipelineState()
{
	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;

	ThrowIfFailed(D3DReadFileToBlob(L"OutlineEdgeVS.cso", &vertexShaderBlob));
	ThrowIfFailed(D3DReadFileToBlob(L"OutlineEdgePS.cso", &pixelShaderBlob));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = { 1, 0 };

	DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void OutlinePass::prepare(const RenderContext& ctx)
{
	if (!ctx.outlineSettings)
	{
		m_enabled = false;
		return;
	}

	m_enabled = ctx.outlineSettings->enabled;

	if (!m_enabled)
	{
		return;
	}

	m_cachedSettings = *ctx.outlineSettings;
	m_viewportWidth = ctx.viewport.Width;
	m_viewportHeight = ctx.viewport.Height;

	m_invProjection = ctx.projection.Invert().Transpose();

	if (!ctx.depthTexture)
	{
		m_enabled = false;
		return;
	}

	m_depthTexture = ctx.depthTexture;
	m_colorTexture = ctx.colorTexture;

	DescriptorHandle srv = ctx.depthTexture->getSRV();
	D3D12_CPU_DESCRIPTOR_HANDLE depthSrcCpu;

	if (srv.IsShaderVisible())
	{
		m_depthSRV = srv.gpu;
		depthSrcCpu = srv.cpu;
	}
	else
	{
		releaseManualSRV();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;

		auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_manualSRV = heap.allocate();

		m_device->CreateShaderResourceView(
			m_depthTexture->getD3D12Resource().Get(),
			&srvDesc,
			m_manualSRV.cpu);

		m_depthSRV = m_manualSRV.gpu;
		depthSrcCpu = m_manualSRV.cpu;
		m_hasManualSRV = true;
	}

	if (m_colorTexture)
	{
		ensureColorCopy(
			static_cast<uint32_t>(m_viewportWidth),
			static_cast<uint32_t>(m_viewportHeight),
			DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	ensureFallbackTexture();

	D3D12_CPU_DESCRIPTOR_HANDLE colorSrcCpu = m_colorTexture ? m_sceneColorCopySRV.cpu : m_fallbackSRV.cpu;

	loadNoiseTexture(m_cachedSettings.noiseTextureAssetId);

	D3D12_CPU_DESCRIPTOR_HANDLE noiseSrcCpu;
	if (m_loadedNoiseTexture)
	{
		DescriptorHandle noiseHandle = m_loadedNoiseTexture->getSRV();
		if (noiseHandle.IsShaderVisible())
		{
			m_noiseSRV = noiseHandle;
			m_hasManualNoiseSRV = false;
			noiseSrcCpu = noiseHandle.cpu;
		}
		else
		{
			if (m_hasManualNoiseSRV)
			{
				auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				heap.free(m_noiseSRV.handle);
			}

			auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_noiseSRV = heap.allocate();

			m_device->CreateShaderResourceView(
				m_loadedNoiseTexture->getD3D12Resource().Get(),
				nullptr,
				m_noiseSRV.cpu);

			m_hasManualNoiseSRV = true;
			noiseSrcCpu = m_noiseSRV.cpu;
		}
	}
	else
	{
		m_noiseSRV = m_fallbackSRV;
		m_hasManualNoiseSRV = false;
		noiseSrcCpu = m_fallbackSRV.cpu;
	}

	if (!m_contiguousSRVBlock)
	{
		auto& heap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_contiguousSRVBlock = heap.allocateBlock(3);
	}

	m_device->CopyDescriptorsSimple(1,
		m_contiguousSRVBlock->getCPUHandle(0),
		depthSrcCpu,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE colorDest = m_contiguousSRVBlock->getCPUHandle(0);
	colorDest.ptr += descriptorSize;
	m_device->CopyDescriptorsSimple(1, colorDest, colorSrcCpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE noiseDest = colorDest;
	noiseDest.ptr += descriptorSize;
	m_device->CopyDescriptorsSimple(1, noiseDest, noiseSrcCpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_srvTableGpu = m_contiguousSRVBlock->getGPUHandle(0);
}

void OutlinePass::apply(ID3D12GraphicsCommandList4* commandList)
{
	if (!m_enabled || !m_depthTexture || m_depthSRV.ptr == 0)
	{
		return;
	}

	CD3DX12_RESOURCE_BARRIER barrierDepthIn = CD3DX12_RESOURCE_BARRIER::Transition(
		m_depthTexture->getD3D12Resource().Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &barrierDepthIn);

	if (m_colorTexture && m_sceneColorCopy)
	{
		CD3DX12_RESOURCE_BARRIER barrierColorToCopy = CD3DX12_RESOURCE_BARRIER::Transition(
			m_colorTexture->getD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->ResourceBarrier(1, &barrierColorToCopy);

		commandList->CopyResource(m_sceneColorCopy.Get(), m_colorTexture->getD3D12Resource().Get());

		CD3DX12_RESOURCE_BARRIER barrierColorToRTV = CD3DX12_RESOURCE_BARRIER::Transition(
			m_colorTexture->getD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrierColorToRTV);
	}

	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* heaps[] = {
		app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(),
		app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap()
	};
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	float texelSizeX = 1.0f / m_viewportWidth;
	float texelSizeY = 1.0f / m_viewportHeight;

	float constants[28] = {
		m_cachedSettings.colorModifier.x,
		m_cachedSettings.colorModifier.y,
		m_cachedSettings.colorModifier.z,
		m_cachedSettings.colorModifier.w,
		texelSizeX,
		texelSizeY,
		m_cachedSettings.minSeparation,
		m_cachedSettings.maxSeparation,
		m_cachedSettings.minDistance,
		m_cachedSettings.maxDistance,
		*(float*)&m_cachedSettings.searchSize,
		m_cachedSettings.noiseScale
	};
	memcpy(&constants[12], &m_invProjection, sizeof(Matrix));
	commandList->SetGraphicsRoot32BitConstants(0, 28, constants, 0);

	commandList->SetGraphicsRootDescriptorTable(1, m_srvTableGpu);

	commandList->SetGraphicsRootDescriptorTable(2,
		app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
			.getGPUHandle(ModuleDescriptors::SampleType::POINT_CLAMP));

	commandList->SetGraphicsRootDescriptorTable(3,
		app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
			.getGPUHandle(ModuleDescriptors::SampleType::LINEAR_CLAMP));

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 0, nullptr);
	commandList->DrawInstanced(3, 1, 0, 0);

	CD3DX12_RESOURCE_BARRIER barrierDepthOut = CD3DX12_RESOURCE_BARRIER::Transition(
		m_depthTexture->getD3D12Resource().Get(),
		D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->ResourceBarrier(1, &barrierDepthOut);
}
