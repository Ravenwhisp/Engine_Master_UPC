#include "Globals.h"
#include "Application.h"
#include "D3D12Module.h"
#include "ResourcesModule.h"
#include "DescriptorsModule.h"
#include "Resources.h"
#include "Globals.h"

Texture::Texture(ID3D12Device4& device, TextureInitInfo info) : Resource(device, *info.desc, &info.clearValue) 
{
    _srv = app->GetDescriptorsModule()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).Allocate();
    device.CreateShaderResourceView(GetD3D12Resource().Get(), info.srvDesc, _srv.cpu);
}

void Texture::Release()
{
    app->GetDescriptorsModule()->DefferDescriptorRelease((Handle)_srv.index);
    app->GetResourcesModule()->DefferResourceRelease(GetD3D12Resource());
}

RenderTexture::RenderTexture(ID3D12Device4& device, TextureInitInfo info) : Texture(device, info) {

    assert(info.desc);
    _mipCount = GetD3D12ResourceDesc().MipLevels;
    assert(_mipCount && _mipCount <= Texture::maxMips);

    D3D12_RENDER_TARGET_VIEW_DESC desc{};
    desc.Format = info.desc->Format;
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;

    for (int i{ 0 }; i < _mipCount; i++) {
        _rtv[i] = app->GetDescriptorsModule()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).Allocate();
        device.CreateRenderTargetView(GetD3D12Resource().Get(), &desc, _rtv[i].cpu);
        ++desc.Texture2D.MipSlice;
    }
}


void RenderTexture::Release()
{
    for (uint32_t i = 0; i < _mipCount; ++i) {
        app->GetDescriptorsModule()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).Free(_rtv[i].index);
    }
    _mipCount = 0;
}


DepthBuffer::DepthBuffer(ID3D12Device4& device, TextureInitInfo info) : Texture(device, info) {

    const DXGI_FORMAT format{ info.desc->Format };

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
    dsv_desc.Format = format;
    dsv_desc.Texture2D.MipSlice = 0;

    _dsv = app->GetDescriptorsModule()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).Allocate();
    device.CreateDepthStencilView(GetD3D12Resource().Get(), &dsv_desc, _dsv.cpu);
}

void DepthBuffer::Release()
{
    app->GetDescriptorsModule()->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).Free(_dsv.index);
}

void Resource::SetName(const std::wstring& name)
{
    m_Name = name;
    m_Resource->SetName(name.c_str());
}

Resource::Resource(ID3D12Device4& device, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue) : device(device) 
{
    if (clearValue &&
        (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
            || resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {

        m_ClearValue = std::make_unique<CD3DX12_CLEAR_VALUE>(*clearValue);
    }
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
        initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device.CreateCommittedResource(
        &heap, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        initialState, m_ClearValue.get(), IID_PPV_ARGS(&m_Resource));
}

Resource::Resource(ID3D12Device4& device, ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue) : m_Resource(resource), device(device) 
{
    if (clearValue)
    {
        m_ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }
}
