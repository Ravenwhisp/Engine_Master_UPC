#include "Globals.h"
#include "UIImagePass.h"
#include "Application.h"
#include "D3D12Module.h"
#include "CommandQueue.h"
#include <DirectXColors.h>
#include <algorithm>

UIImagePass::UIImagePass(ComPtr<ID3D12Device4> device): m_device(device)
{
    m_upload = std::make_unique<ResourceUploadBatch>(device.Get());

    m_upload->Begin();

    RenderTargetState rtState(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);

    const SpriteBatchPipelineStateDescription pd(rtState);

    m_spriteBatch = std::make_unique<SpriteBatch>(m_device.Get(), *m_upload, pd);

    auto uploadFinished = m_upload->End( app->getD3D12Module()->getCommandQueue()->getD3D12CommandQueue().Get());

    uploadFinished.wait();
}

void UIImagePass::apply(ID3D12GraphicsCommandList4* commandList)
{
    if (!m_commands || m_commands->empty())
    { 
        return;
    }

    begin(commandList);

    for (const auto& command : *m_commands)
    {
        drawImage(command);
    }

    end();
}

void UIImagePass::begin(ID3D12GraphicsCommandList4* commandList)
{
    if (!m_viewport)
    {
        return;
    }
    m_spriteBatch->SetViewport(*m_viewport);

    auto& heap = app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    ID3D12DescriptorHeap* heaps[] = { heap.getHeap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    m_spriteBatch->Begin(commandList);
}

void UIImagePass::drawImage(const UIImageCommand& command)
{
    if (!command.texture) return;

    const auto srv = command.texture->getSRV();
    if (!srv.IsShaderVisible() || srv.gpu.ptr == 0)
    {
        return;
    }

    auto resource = command.texture->getD3D12Resource();
    if (!resource)
    {
        return;
    }
        
    const auto desc = resource->GetDesc();
    const DirectX::XMUINT2 texSize(static_cast<uint32_t>(desc.Width),static_cast<uint32_t>(desc.Height));

    RECT dst;
    dst.left = (LONG)command.rect.x;
    dst.top = (LONG)command.rect.y;
    dst.right = (LONG)(command.rect.x + command.rect.w);
    dst.bottom = (LONG)(command.rect.y + command.rect.h);

    m_spriteBatch->Draw(srv.gpu, texSize, dst, DirectX::Colors::White);
}

void UIImagePass::end()
{
    m_spriteBatch->End();
}