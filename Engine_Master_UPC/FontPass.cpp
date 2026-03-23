#include "Globals.h"
#include "FontPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleRender.h"
#include "ModuleD3D12.h"
#include "ModuleTime.h"

#include "Settings.h"
#include "CommandQueue.h"

#include "UICommands.h"

FontPass::FontPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
	m_settings = app->getSettings();

	m_fontHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

	m_upload = std::make_unique<ResourceUploadBatch>(device.Get());

	m_upload->Begin();

	RenderTargetState rtState(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT);

	const SpriteBatchPipelineStateDescription pd(rtState);

	m_spriteBatch = std::make_unique<SpriteBatch>(m_device.Get(), *m_upload, pd);

	m_spriteFont = std::make_unique<SpriteFont>(m_device.Get(), *m_upload,
		L"Assets/Fonts/arial.spritefont",
		m_fontHeap->getCPUHandle(0),
		m_fontHeap->getGPUHandle(0));

	auto uploadResourcesFinished = m_upload->End(app->getModuleD3D12()->getCommandQueue()->getD3D12CommandQueue().Get());

	uploadResourcesFinished.wait();
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
	{
		return;
	}

	begin(commandList);

	showDebugInformation();
	
	if (hasCommands)
	{
		for (const auto& command : *m_commands)
		{
			drawText(command.text.c_str(), command.x, command.y, command.color, command.scale);
		}
	}

	end();
}

void FontPass::begin(ID3D12GraphicsCommandList4* commandList)
{
	if (!m_viewport || !m_spriteBatch)
	{
		return;
	}
	m_spriteBatch->SetViewport(*m_viewport);

	ID3D12DescriptorHeap* heaps[] = { m_fontHeap->getHeap() };
	commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

	m_spriteBatch->Begin(commandList);
}

void FontPass::drawText(const wchar_t* text, float x, float y, const DirectX::XMFLOAT4& color, float scale)
{
	if (!m_spriteFont || !text)
	{
		return;
	}
	const DirectX::XMVECTOR tint = DirectX::XMLoadFloat4(&color);

	m_spriteFont->DrawString(m_spriteBatch.get(), text, DirectX::XMFLOAT2(x, y), tint, 0.0f, DirectX::XMFLOAT2(0, 0), scale);
}

void FontPass::end()
{
	m_spriteBatch->End();
}

void FontPass::showDebugInformation() {
	if (m_settings->debugGame.showFPS)
	{
		float deltaTime = app->getModuleTime()->deltaTime();
		float fps = (deltaTime > 0.0f) ? 1.0f / deltaTime : 0.0f;

		wchar_t buffer[64];
		swprintf_s(buffer, L"FPS: %.0f", fps);

		drawText(buffer, 10.0f, 10.0f, DirectX::XMFLOAT4(0.0f, 1.0f, 0.4f, 1.0f), 1.0f);
	}
	if (m_settings->debugGame.showFrametime)
	{
		float deltaTime = app->getModuleTime()->deltaTime();
		float ms = deltaTime * 1000.0f;

		wchar_t buffer[64];
		swprintf_s(buffer, L"Frame time: %.2f ms", ms);

		drawText(buffer, 10.0f, 30.0f, DirectX::XMFLOAT4(0.0f, 1.0f, 0.4f, 1.0f), 1.0f);
	}
	if (m_settings->debugGame.showTrianglesNumber)
	{
		int triangles = app->getModuleRender()->getTriangles();

		wchar_t buffer[64];
		swprintf_s(buffer, L"Triangles: %d", triangles);

		drawText(buffer, 10.0f, 50.0f, DirectX::XMFLOAT4(0.0f, 1.0f, 0.4f, 1.0f), 1.0f);
	}
}