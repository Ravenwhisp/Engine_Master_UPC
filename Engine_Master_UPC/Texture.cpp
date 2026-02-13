#include "Globals.h"
#include "Texture.h"

#include "Application.h"
#include "DescriptorsModule.h"
#include "ResourcesModule.h"

Texture::Texture(ID3D12Device4& device, TextureInitInfo info) : Resource(device, *info.desc, &info.clearValue)
{
    m_srv = app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).allocate();
    device.CreateShaderResourceView(getD3D12Resource().Get(), info.srvDesc, m_srv.cpu);
}

void Texture::release()
{
    app->getDescriptorsModule()->defferDescriptorRelease((Handle)m_srv.handle);
    app->getResourcesModule()->defferResourceRelease(getD3D12Resource());
}