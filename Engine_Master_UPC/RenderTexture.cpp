#include "Globals.h"
#include "RenderTexture.h"

#include "Application.h"
#include "DescriptorsModule.h"
#include "ResourcesModule.h"


RenderTexture::RenderTexture(ID3D12Device4& device, TextureInitInfo info) : Texture(device, info) {

    assert(info.desc);
    m_mipCount = getD3D12ResourceDesc().MipLevels;
    assert(m_mipCount && m_mipCount <= Texture::MAX_MIPS);

    D3D12_RENDER_TARGET_VIEW_DESC desc{};
    desc.Format = info.desc->Format;
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;

    for (int i{ 0 }; i < m_mipCount; i++)
    {
        m_rtv[i] = app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).allocate();
        device.CreateRenderTargetView(getD3D12Resource().Get(), &desc, m_rtv[i].cpu);
        ++desc.Texture2D.MipSlice;
    }
}


void RenderTexture::release()
{
    for (uint32_t i = 0; i < m_mipCount; ++i)
    {
        app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).free(m_rtv[i].handle);
    }
    m_mipCount = 0;
}