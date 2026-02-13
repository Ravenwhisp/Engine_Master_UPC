#include "Globals.h"
#include "DepthBuffer.h"

#include "Application.h"
#include "DescriptorsModule.h"
#include "ResourcesModule.h"


DepthBuffer::DepthBuffer(ID3D12Device4& device, TextureInitInfo info) : Texture(device, info) {

    const DXGI_FORMAT format{ info.desc->Format };

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
    dsv_desc.Format = format;
    dsv_desc.Texture2D.MipSlice = 0;

    m_dsv = app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).allocate();
    device.CreateDepthStencilView(getD3D12Resource().Get(), &dsv_desc, m_dsv.cpu);
}

void DepthBuffer::release()
{
    app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).free(m_dsv.handle);
}