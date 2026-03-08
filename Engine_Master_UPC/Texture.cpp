#include "Globals.h"
#include "Texture.h"

#include "Application.h"
#include "DescriptorsModule.h"
#include "ResourcesModule.h"

Texture::Texture(const UID uid, ID3D12Device4& device, TextureInitInfo info, DescriptorHandle* texturesHandle) : Resource(device, *info.desc, &info.clearValue), ICacheable(uid)
{
    //m_srv = app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).allocate(); //Now all the textures recive a handle to store them contiguously
    device.CreateShaderResourceView(getD3D12Resource().Get(), info.srvDesc, texturesHandle->cpu);
}

void Texture::release()
{
    app->getDescriptorsModule()->defferDescriptorRelease((Handle)m_srv.handle);
    app->getResourcesModule()->defferResourceRelease(getD3D12Resource());
}