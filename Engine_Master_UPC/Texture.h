#pragma once
#include "Resources.h"

class Texture : public Resource 
{
public:
	constexpr static uint32_t MAX_MIPS{ 14 };
	Texture() = default;
	explicit Texture(ID3D12Device4& device, TextureInitInfo info);
	~Texture() { release(); }

	void				release();
	DescriptorHandle	getSRV() { return m_srv; }
private:
	DescriptorHandle m_srv{};
};