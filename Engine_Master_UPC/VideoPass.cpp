#include "Globals.h"
#include "VideoPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "RenderContext.h"
#include "RenderSurface.h"
#include "Texture.h"
#include "VideoPlayback.h"
#include "PlatformHelpers.h"

#include <d3dcompiler.h>
#include <cstring>

VideoPass::VideoPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
	m_descriptorHeap = std::make_unique<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

	createRootSignature();
	createPipelineState();
}

VideoPass::~VideoPass()
{
	if (m_swsContext)
	{
		sws_freeContext(m_swsContext);
		m_swsContext = nullptr;
	}

	releaseUploadBuffers();

	m_videoTexture.Reset();
	m_pipelineState.Reset();
	m_rootSignature.Reset();
	m_descriptorHeap.reset();
}

void VideoPass::prepare(const RenderContext& ctx)
{
	m_renderSurface = &ctx.renderSurface;
	m_viewport = ctx.viewport;
	m_scissorRect = ctx.scissorRect;
}

void VideoPass::prepare(RenderSurface& renderSurface, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect)
{
	m_renderSurface = &renderSurface;
	m_viewport = viewport;
	m_scissorRect = scissorRect;
}

void VideoPass::apply(ID3D12GraphicsCommandList4* commandList)
{
	render(commandList);
}

bool VideoPass::render(ID3D12GraphicsCommandList4* commandList)
{
	if (!commandList || !m_video || !m_renderSurface || !m_pipelineState)
		return false;

	if (m_video->hasNewVideoFrame() || !m_hasFrame)
	{
		AVFrame* frame = m_video->getVideoFrame();

		if (frame && !updateVideoTexture(commandList, frame))
			return false;
	}

	if (!m_hasFrame || !m_videoTexture)
		return false;

	renderVideo(commandList);

	return true;
}

void VideoPass::setVideo(VideoPlayback* video)
{
	if (m_video == video)
		return;

	m_video = video;
	m_hasFrame = false;
}

void VideoPass::createRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE srvRange;
	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_ROOT_PARAMETER rootParams[1] = {};
	rootParams[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.MaxAnisotropy = 1;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParams), rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void VideoPass::createPipelineState()
{
	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"VideoVertexShader.cso", &vertexShaderBlob));

	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"VideoPixelShader.cso", &pixelShaderBlob));

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depthStencilDesc.StencilEnable = FALSE;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = { 1, 0 };

	DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

bool VideoPass::createVideoTexture(uint32_t width, uint32_t height)
{
	if (!width || !height)
		return false;

	if (m_videoTexture)
		app->getModuleD3D12()->waitForGPU();

	m_videoTexture.Reset();
	releaseUploadBuffers();

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	const HRESULT result = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_videoTexture));

	if (FAILED(result))
	{
		DEBUG_ERROR("[Video Pass] Could not create video texture");
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	m_device->CreateShaderResourceView(m_videoTexture.Get(), &srvDesc, m_descriptorHeap->getCPUHandle(0));

	m_width = width;
	m_height = height;
	m_hasFrame = false;
	m_textureInShaderState = false;

	return true;
}

bool VideoPass::createUploadBuffers(UINT64 uploadSize)
{
	if (!uploadSize)
		return false;

	releaseUploadBuffers();

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = uploadSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		HRESULT result = m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_uploadBuffers[i]));

		if (FAILED(result))
		{
			DEBUG_ERROR("[Video Pass] Could not create upload buffer %u", i);
			releaseUploadBuffers();
			return false;
		}

		D3D12_RANGE readRange = { 0, 0 };

		result = m_uploadBuffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedUploadBuffers[i]));

		if (FAILED(result))
		{
			DEBUG_ERROR("[Video Pass] Could not map upload buffer %u", i);
			releaseUploadBuffers();
			return false;
		}
	}

	m_uploadBufferSize = uploadSize;

	return true;
}

void VideoPass::releaseUploadBuffers()
{
	for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		if (!m_uploadBuffers[i])
		{
			m_mappedUploadBuffers[i] = nullptr;
			continue;
		}

		if (m_mappedUploadBuffers[i])
			m_uploadBuffers[i]->Unmap(0, nullptr);

		m_mappedUploadBuffers[i] = nullptr;
		m_uploadBuffers[i].Reset();
	}

	m_uploadBufferSize = 0;
}

bool VideoPass::convertFrame(AVFrame* frame)
{
	if (!frame || frame->width <= 0 || frame->height <= 0)
		return false;

	if (m_width != static_cast<uint32_t>(frame->width) || m_height != static_cast<uint32_t>(frame->height))
	{
		if (!createVideoTexture(static_cast<uint32_t>(frame->width), static_cast<uint32_t>(frame->height)))
			return false;
	}

	m_frameData.resize(static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4);

	m_swsContext = sws_getCachedContext(m_swsContext, frame->width, frame->height, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, nullptr);

	if (!m_swsContext)
		return false;

	uint8_t* destinationData[4] = { m_frameData.data(), nullptr, nullptr, nullptr };
	int destinationLineSize[4] = { static_cast<int>(m_width * 4), 0, 0, 0 };

	return sws_scale(m_swsContext, frame->data, frame->linesize, 0, frame->height, destinationData, destinationLineSize) > 0;
}

bool VideoPass::updateVideoTexture(ID3D12GraphicsCommandList4* commandList, AVFrame* frame)
{
	if (!commandList || !frame || !convertFrame(frame) || !m_videoTexture)
		return false;

	const D3D12_RESOURCE_DESC textureDesc = m_videoTexture->GetDesc();

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	UINT numRows = 0;
	UINT64 rowSize = 0;
	UINT64 uploadSize = 0;

	m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows, &rowSize, &uploadSize);

	if (m_uploadBufferSize < uploadSize && !createUploadBuffers(uploadSize))
		return false;

	const uint32_t frameIndex = app->getModuleD3D12()->getCurrentFrameIndex();

	if (frameIndex >= FRAMES_IN_FLIGHT)
		return false;

	ID3D12Resource* uploadBuffer = m_uploadBuffers[frameIndex].Get();
	uint8_t* mappedData = m_mappedUploadBuffers[frameIndex];

	if (!uploadBuffer || !mappedData)
		return false;

	const size_t sourcePitch = static_cast<size_t>(m_width) * 4;

	for (UINT row = 0; row < numRows; ++row)
	{
		uint8_t* destination = mappedData + footprint.Offset + static_cast<size_t>(row) * footprint.Footprint.RowPitch;
		const uint8_t* source = m_frameData.data() + static_cast<size_t>(row) * sourcePitch;

		std::memcpy(destination, source, sourcePitch);
	}

	if (m_textureInShaderState)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_videoTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}

	D3D12_TEXTURE_COPY_LOCATION destination = {};
	destination.pResource = m_videoTexture.Get();
	destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	D3D12_TEXTURE_COPY_LOCATION source = {};
	source.pResource = uploadBuffer;
	source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	source.PlacedFootprint = footprint;

	commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_videoTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &barrier);

	m_textureInShaderState = true;
	m_hasFrame = true;

	return true;
}

void VideoPass::renderVideo(ID3D12GraphicsCommandList4* commandList)
{
	if (!commandList || !m_renderSurface)
		return;

	auto colorTexture = m_renderSurface->getTexture(RenderSurface::COMPOSITE);

	if (!colorTexture)
		return;

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = colorTexture->getRTV(0).cpu;

	commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap->getHeap() };
	commandList->SetDescriptorHeaps(1, heaps);

	commandList->SetGraphicsRootDescriptorTable(0, m_descriptorHeap->getGPUHandle(0));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}