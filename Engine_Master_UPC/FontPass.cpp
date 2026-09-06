#include "Globals.h"
#include "FontPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleRender.h"
#include "ModuleD3D12.h"
#include "ModuleTime.h"
#include "ModuleFont.h"
#include "ModuleScene.h"

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

		m_settings->debugGame.maxFPS = std::max(m_settings->debugGame.maxFPS, fps);
		m_settings->debugGame.minFPS = std::min(m_settings->debugGame.minFPS, fps);

		wchar_t buffer[64];
		swprintf_s(buffer, L"FPS: %07.2f, %07.2f - %07.2f", fps, m_settings->debugGame.minFPS, m_settings->debugGame.maxFPS);

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
		command.y = 25.0f;
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
		command.y = 40.0f;
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
		command.y = 55.0f;
		command.color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.4f, 1.0f);
		command.scale = 1.0f;
		command.fontId = DEBUG_FONT_ID;
		drawText(commandList, command);
	}

	if (m_settings->debugGame.showRenderTimings)
	{
		std::vector<ModuleRender::RenderPassTiming> timings =
			app->getModuleRender()->getRenderPassTimings();

		std::sort(timings.begin(), timings.end(),
			[](const ModuleRender::RenderPassTiming& lhs, const ModuleRender::RenderPassTiming& rhs)
			{
				return lhs.gpuMs > rhs.gpuMs;
			});

		float totalGpuMs = 0.0f;
		float totalCpuMs = 0.0f;
		for (const ModuleRender::RenderPassTiming& timing : timings)
		{
			totalGpuMs += timing.gpuMs;
			totalCpuMs += timing.cpuMs;
		}

		wchar_t totalBuffer[128];
		swprintf_s(totalBuffer, L"Measured total: GPU %.2f ms | CPU submit %.2f ms", totalGpuMs, totalCpuMs);

		std::wstring timingText = L"Render timings (GPU delayed, sorted by GPU)\n";
		timingText += totalBuffer;
		timingText += L'\n';

		const Application::FrameCpuTimings& frameCpu = app->getFrameCpuTimings();
		wchar_t frameCpuBuffer[192];
		swprintf_s(
			frameCpuBuffer,
			L"Frame CPU: Update %.2f | PreRender %.2f | Render %.2f | PostRender %.2f ms",
			frameCpu.updateMs,
			frameCpu.preRenderMs,
			frameCpu.renderMs,
			frameCpu.postRenderMs);
		timingText += frameCpuBuffer;
		timingText += L'\n';

		for (const ModuleRender::RenderPassTiming& timing : timings)
		{
			const std::wstring name(timing.name.begin(), timing.name.end());
			wchar_t timingBuffer[192];
			swprintf_s(
				timingBuffer,
				L"%-30ls GPU %7.3f ms | CPU %7.3f ms",
				name.c_str(),
				timing.gpuMs,
				timing.cpuMs);
			timingText += timingBuffer;
			timingText += L'\n';
		}

		UITextCommand command;
		command.text = std::move(timingText);
		command.x = 10.0f;
		command.y = 75.0f;
		command.color = DirectX::XMFLOAT4(1.0f, 0.85f, 0.2f, 1.0f);
		command.scale = 0.85f;
		command.fontId = DEBUG_FONT_ID;
		drawText(commandList, command);
	}

	if (m_settings->debugGame.showUpdateTimings)
	{
		std::vector<Application::ModuleUpdateTiming> timings = app->getModuleUpdateTimings();
		std::sort(timings.begin(), timings.end(),
			[](const Application::ModuleUpdateTiming& lhs, const Application::ModuleUpdateTiming& rhs)
			{
				return lhs.cpuMs > rhs.cpuMs;
			});

		float measuredTotalMs = 0.0f;
		for (const Application::ModuleUpdateTiming& timing : timings)
		{
			measuredTotalMs += timing.cpuMs;
		}

		wchar_t totalBuffer[128];
		swprintf_s(totalBuffer, L"Measured modules: %.3f ms | Full Update: %.3f ms",
			measuredTotalMs, app->getFrameCpuTimings().updateMs);

		std::wstring timingText = L"Update timings (CPU, sorted by cost)\n";
		timingText += totalBuffer;
		timingText += L'\n';

		for (const Application::ModuleUpdateTiming& timing : timings)
		{
			if (timing.cpuMs < 0.005f)
			{
				continue;
			}

			std::wstring name;
			for (const char* character = timing.name; *character != '\0'; ++character)
			{
				name.push_back(static_cast<wchar_t>(*character));
			}
			wchar_t timingBuffer[160];
			swprintf_s(timingBuffer, L"%-30ls CPU %7.3f ms", name.c_str(), timing.cpuMs);
			timingText += timingBuffer;
			timingText += L'\n';
		}

		const ModuleScene::DetailedUpdateTimings& sceneTimings =
			app->getModuleScene()->getDetailedUpdateTimings();
		timingText += L"\nScene detail\n";

		wchar_t detailBuffer[224];
		swprintf_s(detailBuffer, L"GameObjects update       %7.3f ms | %u calls",
			sceneTimings.gameObjectsUpdateMs, sceneTimings.gameObjectUpdateCalls);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"GameObjects lateUpdate   %7.3f ms | %u calls",
			sceneTimings.gameObjectsLateUpdateMs, sceneTimings.gameObjectLateUpdateCalls);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Trigger system           %7.3f ms",
			sceneTimings.triggerSystemMs);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Pending remove/release   %7.3f ms | %u / %u items",
			sceneTimings.removePendingMs + sceneTimings.releaseDestroyedMs,
			sceneTimings.pendingRemovalRequests, sceneTimings.releasedDestroyedObjects);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Pending additions        %7.3f ms | %u items",
			sceneTimings.flushPendingMs, sceneTimings.pendingAdditions);
		timingText += detailBuffer;
		timingText += L'\n';

		swprintf_s(detailBuffer, L"Quadtree resolve static  %7.3f ms | %u dirty nodes",
			sceneTimings.staticQuadtreeUpdateMs, sceneTimings.staticQuadtreeDirtyNodes);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Quadtree resolve dynamic %7.3f ms | %u dirty nodes",
			sceneTimings.dynamicQuadtreeUpdateMs, sceneTimings.dynamicQuadtreeDirtyNodes);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Quadtree moves static    %7.3f ms | %u calls",
			sceneTimings.staticQuadtreeMoveMs, sceneTimings.staticQuadtreeMoveCalls);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Quadtree moves dynamic   %7.3f ms | %u calls",
			sceneTimings.dynamicQuadtreeMoveMs, sceneTimings.dynamicQuadtreeMoveCalls);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Frustum queries static   %7.3f ms | %u calls | %u results",
			sceneTimings.staticQuadtreeQueryMs, sceneTimings.staticQuadtreeQueryCalls,
			sceneTimings.staticQuadtreeQueryResults);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Frustum queries dynamic  %7.3f ms | %u calls | %u results",
			sceneTimings.dynamicQuadtreeQueryMs, sceneTimings.dynamicQuadtreeQueryCalls,
			sceneTimings.dynamicQuadtreeQueryResults);
		timingText += detailBuffer;
		timingText += L'\n';
		swprintf_s(detailBuffer, L"Area queries             %7.3f ms | %u calls | %u results",
			sceneTimings.quadtreeAreaQueryMs, sceneTimings.quadtreeAreaQueryCalls,
			sceneTimings.quadtreeAreaQueryResults);
		timingText += detailBuffer;
		timingText += L'\n';

		struct ComponentTimingLine
		{
			ComponentType type;
			float updateMs;
			float lateUpdateMs;
			uint32_t updateCalls;
			uint32_t lateUpdateCalls;
		};

		std::vector<ComponentTimingLine> componentTimings;
		componentTimings.reserve(ModuleScene::COMPONENT_TYPE_COUNT);
		for (size_t i = 0; i < ModuleScene::COMPONENT_TYPE_COUNT; ++i)
		{
			const uint32_t updateCalls = sceneTimings.componentUpdateCalls[i];
			const uint32_t lateUpdateCalls = sceneTimings.componentLateUpdateCalls[i];
			if (updateCalls == 0 && lateUpdateCalls == 0)
			{
				continue;
			}

			componentTimings.push_back({
				static_cast<ComponentType>(i),
				sceneTimings.componentUpdateMs[i],
				sceneTimings.componentLateUpdateMs[i],
				updateCalls,
				lateUpdateCalls });
		}

		std::sort(componentTimings.begin(), componentTimings.end(),
			[](const ComponentTimingLine& lhs, const ComponentTimingLine& rhs)
			{
				return lhs.updateMs + lhs.lateUpdateMs > rhs.updateMs + rhs.lateUpdateMs;
			});

		timingText += L"\nComponents (top CPU costs)\n";
		const size_t componentLines = std::min<size_t>(componentTimings.size(), 8);
		for (size_t i = 0; i < componentLines; ++i)
		{
			const ComponentTimingLine& timing = componentTimings[i];
			const char* typeName = ComponentTypeToString(timing.type);
			std::wstring wideTypeName;
			for (const char* character = typeName; *character != '\0'; ++character)
			{
				wideTypeName.push_back(static_cast<wchar_t>(*character));
			}

			swprintf_s(detailBuffer, L"%-22ls U %7.3f (%u) | L %7.3f (%u)",
				wideTypeName.c_str(), timing.updateMs, timing.updateCalls,
				timing.lateUpdateMs, timing.lateUpdateCalls);
			timingText += detailBuffer;
			timingText += L'\n';
		}

		UITextCommand command;
		command.text = std::move(timingText);
		command.x = m_settings->debugGame.showRenderTimings ? 650.0f : 10.0f;
		command.y = 75.0f;
		command.color = DirectX::XMFLOAT4(0.3f, 0.85f, 1.0f, 1.0f);
		command.scale = 0.85f;
		command.fontId = DEBUG_FONT_ID;
		drawText(commandList, command);
	}

	if (m_settings->debugGame.showScriptProfiler)
	{
		const ModuleScene* sceneModule = app->getModuleScene();
		std::vector<ModuleScene::ScriptTiming> currentTimings = sceneModule->getCurrentScriptTimings();
		std::vector<ModuleScene::ScriptTiming> spikeTimings = sceneModule->getLastSpikeScriptTimings();
		std::vector<ModuleScene::ScriptScopeTiming> scopeTimings = sceneModule->getLastSpikeFrame() > 0
			? sceneModule->getLastSpikeScriptScopeTimings()
			: sceneModule->getCurrentScriptScopeTimings();

		auto sortByTotal = [](std::vector<ModuleScene::ScriptTiming>& values)
		{
			std::sort(values.begin(), values.end(),
				[](const ModuleScene::ScriptTiming& lhs, const ModuleScene::ScriptTiming& rhs)
				{
					return lhs.totalMs > rhs.totalMs;
				});
		};
		sortByTotal(currentTimings);
		sortByTotal(spikeTimings);
		scopeTimings.erase(
			std::remove_if(scopeTimings.begin(), scopeTimings.end(),
				[](const ModuleScene::ScriptScopeTiming& timing)
				{
					return timing.scriptName != "SpikeTrap";
				}),
			scopeTimings.end());
		std::sort(scopeTimings.begin(), scopeTimings.end(),
			[](const ModuleScene::ScriptScopeTiming& lhs, const ModuleScene::ScriptScopeTiming& rhs)
			{
				return lhs.totalMs > rhs.totalMs;
			});

		auto toWide = [](const std::string& value, size_t maxLength)
		{
			const size_t length = std::min(value.size(), maxLength);
			std::wstring result;
			result.reserve(length);
			for (size_t i = 0; i < length; ++i)
			{
				result.push_back(static_cast<wchar_t>(value[i]));
			}
			return result;
		};

		auto appendScriptLines = [&](std::wstring& text, const std::vector<ModuleScene::ScriptTiming>& values)
		{
			const size_t lineCount = std::min<size_t>(values.size(), 8);
			for (size_t i = 0; i < lineCount; ++i)
			{
				const ModuleScene::ScriptTiming& timing = values[i];
				const float averageMs = timing.calls > 0
					? timing.totalMs / static_cast<float>(timing.calls)
					: 0.0f;
				const std::wstring scriptName = toWide(timing.scriptName, 24);
				const std::wstring objectName = toWide(timing.maxGameObjectName, 22);
				wchar_t line[320];
				swprintf_s(line, L"%-24ls T %6.3f | N %3u | Avg %6.3f | Max %6.3f | %ls #%llu",
					scriptName.c_str(), timing.totalMs, timing.calls, averageMs,
					timing.maxMs, objectName.c_str(),
					static_cast<unsigned long long>(timing.maxGameObjectId));
				text += line;
				text += L'\n';
			}
		};

		auto appendScopeLines = [&](std::wstring& text, const std::vector<ModuleScene::ScriptScopeTiming>& values)
		{
			const size_t lineCount = std::min<size_t>(values.size(), 12);
			for (size_t i = 0; i < lineCount; ++i)
			{
				const ModuleScene::ScriptScopeTiming& timing = values[i];
				const float averageMs = timing.calls > 0
					? timing.totalMs / static_cast<float>(timing.calls)
					: 0.0f;
				const std::wstring scopeName = toWide(timing.scopeName, 31);
				const std::wstring objectName = toWide(timing.maxGameObjectName, 17);
				wchar_t line[320];
				swprintf_s(line, L"  %-31ls T %6.3f | N %3u | Avg %6.3f | Max %6.3f | %ls #%llu",
					scopeName.c_str(), timing.totalMs, timing.calls, averageMs, timing.maxMs,
					objectName.c_str(), static_cast<unsigned long long>(timing.maxGameObjectId));
				text += line;
				text += L'\n';
			}
		};

		wchar_t header[192];
		swprintf_s(header, L"Current frame %llu: %.3f ms | %zu script classes",
			static_cast<unsigned long long>(sceneModule->getScriptProfilerFrame()),
			sceneModule->getCurrentScriptTotalMs(), currentTimings.size());

		std::wstring profilerText = L"Script profiler (T total, N calls)\n";
		profilerText += header;
		profilerText += L'\n';
		appendScriptLines(profilerText, currentTimings);

		profilerText += L"\n";
		if (sceneModule->getLastSpikeFrame() > 0)
		{
			swprintf_s(header, L"Last spike frame %llu: %.3f ms (threshold %.1f ms)",
				static_cast<unsigned long long>(sceneModule->getLastSpikeFrame()),
				sceneModule->getLastSpikeScriptTotalMs(),
				sceneModule->getScriptSpikeThresholdMs());
			profilerText += header;
			profilerText += L'\n';
			appendScriptLines(profilerText, spikeTimings);
		}
		else
		{
			swprintf_s(header, L"Waiting for a frame above %.1f ms...",
				sceneModule->getScriptSpikeThresholdMs());
			profilerText += header;
			profilerText += L'\n';
		}

		if (!scopeTimings.empty())
		{
			profilerText += sceneModule->getLastSpikeFrame() > 0
				? L"\nSpikeTrap detail (captured spike)\n"
				: L"\nSpikeTrap detail (current frame)\n";
			appendScopeLines(profilerText, scopeTimings);
		}

		UITextCommand command;
		command.text = std::move(profilerText);
		command.x = 10.0f;
		if (m_settings->debugGame.showRenderTimings) command.x += 650.0f;
		if (m_settings->debugGame.showUpdateTimings) command.x += 650.0f;
		command.y = 75.0f;
		command.color = DirectX::XMFLOAT4(1.0f, 0.45f, 0.75f, 1.0f);
		command.scale = 0.80f;
		command.fontId = DEBUG_FONT_ID;
		drawText(commandList, command);
	}
}
