#pragma once
#include "DescriptorHeap.h"



struct TextureInitInfo {
	ID3D12Heap1* heap{ nullptr };
	ID3D12Resource* resource{ nullptr };
	D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc{};
	D3D12_RESOURCE_DESC* desc{};
	D3D12_RESOURCE_ALLOCATION_INFO1 allocInfo{};
	D3D12_RESOURCE_STATES initialState{ };
	D3D12_CLEAR_VALUE clearValue{  };
};


class Resource {
public:
	ComPtr<ID3D12Resource> GetD3D12Resource() const
	{
		return m_Resource;
	}

	D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
	{
		D3D12_RESOURCE_DESC resDesc = {};
		if (m_Resource)
		{
			resDesc = m_Resource->GetDesc();
		}

		return resDesc;
	}

	void SetName(const std::wstring& name);
	const std::wstring& GetName() const
	{
		return m_Name;
	}
protected:

	Resource(ID3D12Device4& device, const D3D12_RESOURCE_DESC& resourceDesc,
		const D3D12_CLEAR_VALUE* clearValue = nullptr);
	Resource(ID3D12Device4& device, ComPtr<ID3D12Resource> resource,
		const D3D12_CLEAR_VALUE* clearValue = nullptr);

	virtual ~Resource() = default;

	ComPtr<ID3D12Resource> m_Resource;
	D3D12_FEATURE_DATA_FORMAT_SUPPORT m_FormatSupport;
	std::unique_ptr<D3D12_CLEAR_VALUE> m_ClearValue;
	std::wstring m_Name;
	ID3D12Device4& device;
};


class Texture : public Resource {
public:
	constexpr static uint32_t maxMips{ 14 };
	Texture() = default;
	explicit Texture(ID3D12Device4& device, TextureInitInfo info);

	~Texture() {
		Release();
	}
	void Release();
	DescriptorHandle SRV() { return _srv; }
private:
	DescriptorHandle _srv{};
};


class RenderTexture : public Texture {
public:
	RenderTexture() = default;
	explicit RenderTexture(ID3D12Device4& device, TextureInitInfo info);

	~RenderTexture() {
		Release();
	}

	void Release();
	uint32_t MipCount() const { return _mipCount; }
	DescriptorHandle RTV(uint32_t mipIndex) const { assert(mipIndex < _mipCount); return _rtv[mipIndex]; }
private:

	DescriptorHandle _rtv[Texture::maxMips]{};
	uint32_t _mipCount{ 0 };
};

class DepthBuffer : public Texture {
public:
	DepthBuffer() = default;
	explicit DepthBuffer(ID3D12Device4& device, TextureInitInfo info);

	~DepthBuffer() {
		Release();
	}
	void Release();
	DescriptorHandle DSV(){ return _dsv; }
private:
	DescriptorHandle _dsv{};
};

class RenderTarget {
public:
	ComPtr<ID3D12Resource> resource{ nullptr };
	DescriptorHandle rtv{};
};
