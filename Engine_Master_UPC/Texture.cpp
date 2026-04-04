#include "Globals.h"
#include "Texture.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"


namespace
{
    D3D12_RESOURCE_DESC buildResourceDesc(const TextureDesc& desc)
    {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if ((desc.views & TextureView::RTV) != TextureView::None)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        if ((desc.views & TextureView::DSV) != TextureView::None)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        if ((desc.views & TextureView::UAV) != TextureView::None)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        if ((desc.views & TextureView::DSV) != TextureView::None &&
            (desc.views & TextureView::SRV) == TextureView::None)
        {
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }

        return CD3DX12_RESOURCE_DESC::Tex2D(
            desc.format,
            static_cast<UINT64>(desc.width),
            static_cast<UINT>(desc.height),
            desc.arraySize,
            desc.mipLevels,
            /*sampleCount=*/1,
            /*sampleQuality=*/0,
            flags);
    }

}

Texture::Texture(UID uid, ID3D12Device4& device, const TextureDesc& desc) : Resource(device, buildResourceDesc(desc), desc.hasClearValue ? &desc.clearValue : nullptr)
    , ICacheable(uid)
    , m_desc(desc)
{
    // Clamp mip levels to what was actually allocated on the resource
    m_mipCount = static_cast<uint32_t>(getD3D12ResourceDesc().MipLevels);
    assert(m_mipCount > 0 && m_mipCount <= MAX_MIPS);

    createViews();
}

Texture::Texture(UID uid, ID3D12Device4& device, ComPtr<ID3D12Resource> existingResource, TextureView views,  DXGI_FORMAT rtvFormat) : Resource(device, existingResource)
    , ICacheable(uid)
{
    assert(existingResource && "existingResource must not be null");

    // Derive desc fields directly from the resource so TextureDesc is consistent.
    const D3D12_RESOURCE_DESC d = existingResource->GetDesc();

    m_desc.format = d.Format;
    m_desc.width = static_cast<uint32_t>(d.Width);
    m_desc.height = static_cast<uint32_t>(d.Height);
    m_desc.arraySize = static_cast<uint16_t>(d.DepthOrArraySize);
    m_desc.mipLevels = static_cast<uint16_t>(d.MipLevels);
    m_desc.views = views;

    // Caller can specify an explicit RTV format (e.g. UNORM_SRGB over a UNORM resource).
    m_desc.rtvFormat = (rtvFormat != DXGI_FORMAT_UNKNOWN) ? rtvFormat : d.Format;

    m_mipCount = static_cast<uint32_t>(d.MipLevels);
    assert(m_mipCount > 0 && m_mipCount <= MAX_MIPS);

    createViews();
}

Texture::~Texture()
{
    releaseViews();
    app->getModuleResources()->deferResourceRelease(getD3D12Resource());
}

DescriptorHandle Texture::getSRV() const
{
    assert(hasSRV() && "Texture was not created with TextureView::SRV");
    return m_srv;
}

DescriptorHandle Texture::getRTV(uint32_t mip) const
{
    assert(hasRTV() && "Texture was not created with TextureView::RTV");
    assert(mip < m_mipCount && "RTV mip index out of range");
    return m_rtv[mip];
}

DescriptorHandle Texture::getDSV() const
{
    assert(hasDSV() && "Texture was not created with TextureView::DSV");
    return m_dsv;
}

DescriptorHandle Texture::getUAV(uint32_t mip) const
{
    assert(hasUAV() && "Texture was not created with TextureView::UAV");
    assert(mip < m_mipCount && "UAV mip index out of range");
    return m_uav[mip];
}

DescriptorHandle Texture::getContiguousRTV(uint32_t index) const
{
    //Put Assert Here.
    return m_contiguousRTV->getHandle(index);
}


bool Texture::resize(uint32_t newWidth, uint32_t newHeight)
{
    assert(newWidth > 0 && "Resize width must be > 0");
    assert(newHeight > 0 && "Resize height must be > 0");

    if (newWidth == m_desc.width && newHeight == m_desc.height)
        return true; // nothing to do

    // 1. Release all descriptor handles (deferred-safe: we keep the
    //    ComPtr alive until we overwrite m_Resource below)
    releaseViews();

    // 2. Defer the old underlying resource — GPU may still be reading it
    app->getModuleResources()->deferResourceRelease(m_Resource);
    m_Resource.Reset();

    // 3. Patch the desc and rebuild
    m_desc.width = newWidth;
    m_desc.height = newHeight;

    D3D12_RESOURCE_DESC resourceDesc = buildResourceDesc(m_desc);
    const D3D12_CLEAR_VALUE* pClear = m_desc.hasClearValue ? &m_desc.clearValue : nullptr;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    HRESULT hr = m_device.CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        m_desc.initialState,
        pClear,
        IID_PPV_ARGS(&m_Resource));

    if (FAILED(hr))
    {
        assert(false && "Texture::resize — CreateCommittedResource failed");
        return false;
    }

    // 4. Refresh mip count (mip levels may differ after resize if auto-computed)
    m_mipCount = static_cast<uint32_t>(getD3D12ResourceDesc().MipLevels);

    // 5. Recreate all views on the new resource
    createViews();

    return true;
}


void Texture::allocateResource()
{
    D3D12_RESOURCE_DESC resourceDesc = buildResourceDesc(m_desc);
    const D3D12_CLEAR_VALUE* pClear = m_desc.hasClearValue ? &m_desc.clearValue : nullptr;
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    m_device.CreateCommittedResource(
        &heapProperties,

        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        m_desc.initialState,
        pClear,
        IID_PPV_ARGS(&m_Resource));
}

void Texture::createViews()
{
    if (hasSRV()) createSRV();
    if (hasRTV()) createRTV();
    if (hasDSV()) createDSV();
    if (hasUAV()) createUAV();
}


void Texture::createSRV()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = resolvedSRVFormat();
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (m_desc.arraySize == 6)
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = m_mipCount;
        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else if (m_desc.arraySize > 1)
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = m_mipCount;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = m_desc.arraySize;
        srvDesc.Texture2DArray.PlaneSlice = 0;
        srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = m_mipCount;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    }

    if (m_desc.shaderVisibleSRV)
    {
        // Render targets and other textures bound directly by GPU handle.
        // Allocate in the shader-visible heap — .gpu is populated and valid.
        DescriptorHeapBlock* block = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).allocateBlock(1);

        m_srv = block->getHandle(0);
    }
    else
    {
        // Asset textures that will be copied into a material block.
        // Allocate in the CPU-only staging heap — .gpu is zeroed, .cpu is valid.
        m_srv.cpu = app->getModuleDescriptors()->getStagingHeap().allocate();
        m_srv.gpu = {};
        m_srv.block = nullptr;
    }
    m_device.CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_srv.cpu);
}


void Texture::createRTV()
{
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

    rtvDesc.Format = resolvedRTVFormat();

    if (m_desc.arraySize == 6)
    {
        
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.ArraySize = 1;
        rtvDesc.Texture2DArray.MipSlice = 0;
        rtvDesc.Texture2DArray.PlaneSlice = 0;

        m_contiguousRTV = std::make_unique<DescriptorHeapBlock>(app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).allocateBlock(m_desc.arraySize));

        for (size_t i = 0; i < m_desc.arraySize; i++)
        {
            rtvDesc.Texture2DArray.FirstArraySlice = i;

            m_device.CreateRenderTargetView(m_Resource.Get(), &rtvDesc, m_contiguousRTV->getCPUHandle(i));
        }
    }
    else
    {
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.PlaneSlice = 0;

        for (uint32_t mip = 0; mip < m_mipCount; ++mip)
        {
            rtvDesc.Texture2D.MipSlice = mip;

            m_rtv[mip] = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).allocate();

            m_device.CreateRenderTargetView(m_Resource.Get(), &rtvDesc, m_rtv[mip].cpu);
        }
    }

    
}


void Texture::createDSV()
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = resolvedDSVFormat();
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.Texture2D.MipSlice = 0;

    m_dsv = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).allocate();

    m_device.CreateDepthStencilView(m_Resource.Get(), &dsvDesc, m_dsv.cpu);
}

void Texture::createUAV()
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = resolvedUAVFormat();
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        uavDesc.Texture2D.MipSlice = mip;
        uavDesc.Texture2D.PlaneSlice = 0;

        m_uav[mip] = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).allocate();

        m_device.CreateUnorderedAccessView(
        m_Resource.Get(), /*pCounterResource=*/nullptr, &uavDesc, m_uav[mip].cpu);
    }
}


void Texture::releaseViews()
{
    auto* descriptors = app->getModuleDescriptors();

    if (hasSRV() && m_srv.IsValid() != 0)
    {
        if (m_desc.shaderVisibleSRV)
        {
            // Shader-visible block — defer release (GPU may still be reading it)
            app->getModuleDescriptors()->defferDescriptorRelease((Handle)m_srv.handle);
        }
        else
        {
            // Staging slot — CPU-only, safe to free immediately
            app->getModuleDescriptors()->getStagingHeap().free(m_srv.cpu);
        }       
        m_srv = {};
    }

    if (hasRTV())
    {
        for (uint32_t mip = 0; mip < m_mipCount; ++mip)
        {
            if (m_rtv[mip].IsValid())
            {
                descriptors->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).free(m_rtv[mip].handle);
                m_rtv[mip] = {};
            }
        }
    }

    if (hasDSV() && m_dsv.IsValid())
    {
        descriptors->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
            .free(m_dsv.handle);
        m_dsv = {};
    }

    if (hasUAV())
    {
        for (uint32_t mip = 0; mip < m_mipCount; ++mip)
        {
            if (m_uav[mip].IsValid())
            {
                descriptors->defferDescriptorRelease((Handle)m_uav[mip].handle);
                m_uav[mip] = {};
            }
        }
    }
}


DXGI_FORMAT Texture::resolvedSRVFormat() const
{
    if (m_desc.srvFormat != DXGI_FORMAT_UNKNOWN)
        return m_desc.srvFormat;

    // Auto-remap typeless depth formats to their float equivalent
    switch (m_desc.format)
    {
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_TYPELESS:  return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_TYPELESS:  return DXGI_FORMAT_R16_UNORM;
    default: break;
    }

    return m_desc.format;
}

DXGI_FORMAT Texture::resolvedRTVFormat() const
{
    if (m_desc.rtvFormat != DXGI_FORMAT_UNKNOWN)
        return m_desc.rtvFormat;

    return m_desc.format;
}

DXGI_FORMAT Texture::resolvedDSVFormat() const
{
    if (m_desc.dsvFormat != DXGI_FORMAT_UNKNOWN)
        return m_desc.dsvFormat;

    switch (m_desc.format)
    {
    case DXGI_FORMAT_R32_TYPELESS:  return DXGI_FORMAT_D32_FLOAT;
    case DXGI_FORMAT_R16_TYPELESS:  return DXGI_FORMAT_D16_UNORM;
    default: break;
    }

    return m_desc.format;
}

DXGI_FORMAT Texture::resolvedUAVFormat() const
{
    if (m_desc.uavFormat != DXGI_FORMAT_UNKNOWN)
    {
        return m_desc.uavFormat;
    }

    return m_desc.format;
}