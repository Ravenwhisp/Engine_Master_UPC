#pragma once
#include "Resources.h"

class Buffer: public Resource
{
public:
protected:
	Buffer(ID3D12Device4& device, const D3D12_RESOURCE_DESC& resDesc);
	Buffer(ID3D12Device4& device, ComPtr<ID3D12Resource> resource);
};

