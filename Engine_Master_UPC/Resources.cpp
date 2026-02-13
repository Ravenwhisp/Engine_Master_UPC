#include "Globals.h"
#include "Resources.h"

#include "Application.h"
#include "ResourcesModule.h"
#include "DescriptorsModule.h"

D3D12_RESOURCE_DESC Resource::getD3D12ResourceDesc() const
{
    D3D12_RESOURCE_DESC resDesc = {};
    if (m_Resource)
    {
        resDesc = m_Resource->GetDesc();
    }

    return resDesc;
}

void Resource::setName(const std::wstring& name)
{
    m_Name = name;
    m_Resource->SetName(name.c_str());
}

Resource::Resource(ID3D12Device4& device, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue) : m_device(device) 
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

Resource::Resource(ID3D12Device4& device, ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue) : m_Resource(resource), m_device(device) 
{
    if (clearValue)
    {
        m_ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }
}
