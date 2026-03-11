#pragma once
#include "DescriptorHeap.h"

struct TextureInitInfo {
	ID3D12Heap1*						heap{ nullptr };
	ID3D12Resource*						resource{ nullptr };
	D3D12_SHADER_RESOURCE_VIEW_DESC*	srvDesc{};
	D3D12_RESOURCE_DESC*				desc{};
	D3D12_RESOURCE_ALLOCATION_INFO1		allocInfo{};
	D3D12_RESOURCE_STATES				initialState{ };
	D3D12_CLEAR_VALUE					clearValue{  };
};

struct RenderTarget {
	ComPtr<ID3D12Resource>	resource{ nullptr };
	DescriptorHandle		rtv{};
};


class Resource {
public:
	ComPtr<ID3D12Resource>	getD3D12Resource() const { return m_Resource; }
	D3D12_RESOURCE_DESC		getD3D12ResourceDesc() const;

	void					setName(const std::wstring& name);
	const std::wstring&		getName() const { return m_Name; }
protected:

	Resource(ID3D12Device4& device, const D3D12_RESOURCE_DESC& resourceDesc,
		const D3D12_CLEAR_VALUE* clearValue = nullptr);
	Resource(ID3D12Device4& device, ComPtr<ID3D12Resource> resource,
		const D3D12_CLEAR_VALUE* clearValue = nullptr);

	virtual ~Resource() = default;

	ComPtr<ID3D12Resource>					m_Resource;
	D3D12_FEATURE_DATA_FORMAT_SUPPORT		m_FormatSupport;
	std::unique_ptr<D3D12_CLEAR_VALUE>		m_ClearValue;
	std::wstring							m_Name;
	ID3D12Device4&							m_device;
};









