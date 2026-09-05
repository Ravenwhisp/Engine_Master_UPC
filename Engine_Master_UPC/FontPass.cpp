#include "Globals.h"
#include "FontPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleRender.h"
#include "ModuleD3D12.h"
#include "ModuleTime.h"
#include "ModuleFont.h"

#include "Settings.h"
#include "CommandQueue.h"

#include "UICommands.h"

#include "PlatformHelpers.h"
#include "DescriptorHeap.h"

#include <d3dcompiler.h>
#include <SpriteFont.h>
#include <algorithm>

constexpr int DEBUG_FONT_ID = 0;

FontPass::FontPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
	m_settings = app->getSettings();

	m_fontHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

	m_upload = std::make_unique<ResourceUploadBatch>(device.Get());

	m_upload->Begin();

	app->getModuleFont()->loadFonts(m_device.Get(), *m_upload, *m_fontHeap);

	auto uploadResourcesFinished = m_upload->End(app->getModuleD3D12()->getCommandQueue()->getD3D12CommandQueue().Get());

	uploadResourcesFinished.wait();

	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};

	CD3DX12_DESCRIPTOR_RANGE srvRange;
	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(2, rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"FontVertexShader.cso", vertexShaderBlob.GetAddressOf()));
	ThrowIfFailed(D3DReadFileToBlob(L"FontPixelShader.cso", pixelShaderBlob.GetAddressOf()));

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
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

	psoDesc.NodeMask = 0;
	psoDesc.CachedPSO = {};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

	if (FAILED(hr))
	{
		DEBUG_LOG("FontPass PSO failed: 0x%08X", hr);
		DXCall(hr);
	}
}

void FontPass::prepare(const RenderContext& ctx)
{
	m_viewport = &ctx.viewport;
	m_commands = ctx.uiTextCommands;
}

void FontPass::apply(ID3D12GraphicsCommandList4* commandList)
{
	bool hasCommands = m_commands && !m_commands->empty();
	bool hasDebug = m_settings->hasDebugInformationEnabled();

	if (!hasCommands && !hasDebug)
		return;

	begin(commandList);

	showDebugInformation(commandList);

	if (hasCommands)
	{
		for (const auto& command : *m_commands)
		{
			drawText(commandList, command);
		}
	}

	end(commandList);
}

void FontPass::begin(ID3D12GraphicsCommandList4* commandList)
{
	if (!m_viewport || !m_pipelineState || !m_rootSignature)
		return;

	m_time += app->getModuleTime()->deltaTime();

	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* heaps[] = { m_fontHeap->getHeap() };
	commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FontPass::drawText(ID3D12GraphicsCommandList4* commandList, const UITextCommand& command)
{
	if (command.effectFlags & UITextEffect_Shadow)
	{
		UITextCommand shadowCommand = command;
		shadowCommand.x += command.shadowOffsetX;
		shadowCommand.y += command.shadowOffsetY;
		shadowCommand.color = command.shadowColor;
		shadowCommand.effectFlags &= ~UITextEffect_Shadow;
		shadowCommand.effectFlags &= ~UITextEffect_Outline;
		shadowCommand.effectFlags &= ~UITextEffect_Glow;

		drawTextInternal(commandList, shadowCommand);
	}

	drawTextInternal(commandList, command);
}

void FontPass::drawTextInternal(ID3D12GraphicsCommandList4* commandList, const UITextCommand& command)
{
	if (command.text.empty())
		return;

	const int fontId = command.fontId;

	D3D12_GPU_DESCRIPTOR_HANDLE atlas = app->getModuleFont()->getFontTexture(fontId);
	DirectX::XMUINT2 atlasSize = app->getModuleFont()->getFontTextureSize(fontId);

	if (atlas.ptr == 0 || atlasSize.x == 0 || atlasSize.y == 0)
		return;

	m_vertices.clear();

	const float lineSpacing = app->getModuleFont()->getLineSpacing(fontId);

	float cursorX = command.x;
	float cursorY = command.y + lineSpacing * command.scale;

	const float invAtlasW = 1.0f / static_cast<float>(atlasSize.x);
	const float invAtlasH = 1.0f / static_cast<float>(atlasSize.y);

	const bool hasOutline = (command.effectFlags & UITextEffect_Outline) != 0;
	const bool hasGlow = (command.effectFlags & UITextEffect_Glow) != 0;

	const float paddingPixels = std::max(
		hasOutline ? command.outlineSize : 0.0f,
		hasGlow ? command.glowSize : 0.0f
	);

	for (const wchar_t* c = command.text.c_str(); *c != L'\0'; ++c)
	{
		if (*c == L'\r')
			continue;

		if (*c == L'\n')
		{
			cursorX = command.x;
			cursorY += lineSpacing * command.scale;
			continue;
		}

		const SpriteFont::Glyph* glyph = app->getModuleFont()->getGlyph(fontId, *c);

		if (!glyph)
			continue;

		const float glyphW = static_cast<float>(glyph->Subrect.right - glyph->Subrect.left);
		const float glyphH = static_cast<float>(glyph->Subrect.bottom - glyph->Subrect.top);

		const float pad = paddingPixels * command.scale;

		const float left = cursorX + glyph->XOffset * command.scale - pad;
		const float top = cursorY + (glyph->YOffset - lineSpacing) * command.scale - pad;
		const float right = left + glyphW * command.scale + pad * 2.0f;
		const float bottom = top + glyphH * command.scale + pad * 2.0f;

		const float u0 = (static_cast<float>(glyph->Subrect.left) - paddingPixels) * invAtlasW;
		const float v0 = (static_cast<float>(glyph->Subrect.top) - paddingPixels) * invAtlasH;
		const float u1 = (static_cast<float>(glyph->Subrect.right) + paddingPixels) * invAtlasW;
		const float v1 = (static_cast<float>(glyph->Subrect.bottom) + paddingPixels) * invAtlasH;

		if (glyphW > 0.0f && glyphH > 0.0f)
		{
			m_vertices.push_back({ DirectX::XMFLOAT2(left, top), DirectX::XMFLOAT2(u0, v0), command.color });
			m_vertices.push_back({ DirectX::XMFLOAT2(right, top), DirectX::XMFLOAT2(u1, v0), command.color });
			m_vertices.push_back({ DirectX::XMFLOAT2(right, bottom), DirectX::XMFLOAT2(u1, v1), command.color });

			m_vertices.push_back({ DirectX::XMFLOAT2(left, top), DirectX::XMFLOAT2(u0, v0), command.color });
			m_vertices.push_back({ DirectX::XMFLOAT2(right, bottom), DirectX::XMFLOAT2(u1, v1), command.color });
			m_vertices.push_back({ DirectX::XMFLOAT2(left, bottom), DirectX::XMFLOAT2(u0, v1), command.color });
		}

		cursorX += (glyphW + glyph->XAdvance + glyph->XOffset) * command.scale;
	}

	if (m_vertices.empty())
		return;

	FontParams params{};
	params.viewportSize = DirectX::XMFLOAT2(m_viewport->Width, m_viewport->Height);
	params.atlasTexelSize = DirectX::XMFLOAT2(invAtlasW, invAtlasH);
	params.time = m_time;
	params.effectFlags = command.effectFlags;
	params.outlineSize = command.outlineSize;
	params.glowSize = command.glowSize;
	params.outlineColor = command.outlineColor;
	params.glowColor = command.glowColor;
	params.waveAmplitude = command.waveAmplitude;
	params.waveFrequency = command.waveFrequency;
	params.waveSpeed = command.waveSpeed;

	commandList->SetGraphicsRootConstantBufferView(0, app->getModuleRender()->allocateInRingBuffer(&params, sizeof(FontParams)));

	const UINT vertexBufferSize = static_cast<UINT>(m_vertices.size() * sizeof(FontVertex));
	const D3D12_GPU_VIRTUAL_ADDRESS vertexBufferAddress = app->getModuleRender()->allocateInRingBuffer(m_vertices.data(), vertexBufferSize);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexBufferAddress;
	vertexBufferView.SizeInBytes = vertexBufferSize;
	vertexBufferView.StrideInBytes = sizeof(FontVertex);

	commandList->SetGraphicsRootDescriptorTable(1, atlas);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->DrawInstanced(static_cast<UINT>(m_vertices.size()), 1, 0, 0);
}

void FontPass::end(ID3D12GraphicsCommandList4* commandList) { }

void FontPass::showDebugInformation(ID3D12GraphicsCommandList4* commandList)
{
	if (m_settings->debugGame.showFPS)
	{
		float deltaTime = app->getModuleTime()->deltaTime();
		float fps = (deltaTime > 0.0f) ? 1.0f / deltaTime : 0.0f;

		wchar_t buffer[64];
		swprintf_s(buffer, L"FPS: %.0f", fps);

		UITextCommand command;
		command.text = buffer;
		command.x = 10.0f;
		command.y = 10.0f;
		command.color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.4f, 1.0f);
		command.scale = 1.0f;
		command.fontId = DEBUG_FONT_ID;
		drawText(commandList, command);
	}

	if (m_settings->debugGame.showFrametime)
	{
		float deltaTime = app->getModuleTime()->deltaTime();
		float ms = deltaTime * 1000.0f;

		wchar_t buffer[64];
		swprintf_s(buffer, L"Frame time: %.2f ms", ms);

		UITextCommand command;
		command.text = buffer;
		command.x = 10.0f;
		command.y = 10.0f;
		command.color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.4f, 1.0f);
		command.scale = 1.0f;
		command.fontId = DEBUG_FONT_ID;
		drawText(commandList, command);
	}

	if (m_settings->debugGame.showTrianglesNumber)
	{
		int triangles = app->getModuleRender()->getTrianglesCount();

		wchar_t buffer[64];
		swprintf_s(buffer, L"Triangles: %d", triangles);

		UITextCommand command;
		command.text = buffer;
		command.x = 10.0f;
		command.y = 10.0f;
		command.color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.4f, 1.0f);
		command.scale = 1.0f;
		command.fontId = DEBUG_FONT_ID;
		drawText(commandList, command);
	}

	if (m_settings->debugGame.showMeshNumber)
	{
		int meshes = app->getModuleRender()->getMeshCount();

		wchar_t buffer[64];
		swprintf_s(buffer, L"Meshes: %d", meshes);

		UITextCommand command;
		command.text = buffer;
		command.x = 10.0f;
		command.y = 10.0f;
		command.color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.4f, 1.0f);
		command.scale = 1.0f;
		command.fontId = DEBUG_FONT_ID;
		drawText(commandList, command);
	}
}