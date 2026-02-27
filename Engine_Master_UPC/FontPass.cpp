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
	m_spriteBatch->SetViewport(*m_viewport);

	ID3D12DescriptorHeap* heaps[] = { m_fontHeap->getHeap() };
	commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

	m_spriteBatch->Begin(commandList);

	//Here we create all the sprite batch and fonts
	//m_spriteBatch->Draw(resourceDescriptors->GetGpuHandle(Descriptors::MySpriteTexture), GetTextureSize(tex), XMFLOAT2(x, y));
	m_spriteFont->DrawString(m_spriteBatch.get(), L"RAVENWHISP!", XMFLOAT2(30, 30));

	m_spriteBatch->End();
}
