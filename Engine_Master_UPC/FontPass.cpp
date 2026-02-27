#include "Globals.h"
#include "FontPass.h"
#include "Application.h"
#include "D3D12Module.h"
#include "CommandQueue.h"

FontPass::FontPass(ComPtr<ID3D12Device4> device): m_device(device)
{
	m_fontHeap = std::make_unique<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

	m_upload =  std::make_unique<ResourceUploadBatch>(device.Get());

	m_upload->Begin();

	RenderTargetState rtState(DXGI_FORMAT_R8G8B8A8_UNORM , DXGI_FORMAT_D32_FLOAT);

	const SpriteBatchPipelineStateDescription pd(rtState);

	auto upload2 = m_upload.get();

	m_spriteBatch = std::make_unique<SpriteBatch>(m_device.Get(), *m_upload, pd);

	m_spriteFont = std::make_unique<SpriteFont>(m_device.Get(), *m_upload,
		L"Assets/Fonts/arial.spritefont",
		m_fontHeap->getCPUHandle(0),
		m_fontHeap->getGPUHandle(0));

	auto uploadResourcesFinished = m_upload->End(app->getD3D12Module()->getCommandQueue()->getD3D12CommandQueue().Get());

	uploadResourcesFinished.wait();
}

void FontPass::apply(ID3D12GraphicsCommandList4* commandList)
{
	begin(commandList);
	drawText(L"RAVENWHISP!", 20, 20);
	end();
}

void FontPass::begin(ID3D12GraphicsCommandList4* commandList)
{
	if (!m_viewport)
	{
		return;
	}
	m_spriteBatch->SetViewport(*m_viewport);

	ID3D12DescriptorHeap* heaps[] = { m_fontHeap->getHeap() };
	commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

	m_spriteBatch->Begin(commandList);
}

void FontPass::drawText(const wchar_t* text, float x, float y)
{
	if (!m_spriteFont || !text)
	{
		return;
	}
	m_spriteFont->DrawString(m_spriteBatch.get(), text, DirectX::XMFLOAT2(x, y));
}

void FontPass::end()
{
	m_spriteBatch->End();
}
