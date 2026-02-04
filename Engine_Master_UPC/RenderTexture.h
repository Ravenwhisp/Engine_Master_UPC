#pragma once
#include "Texture.h"

class RenderTexture : public Texture {
public:
	RenderTexture() = default;
	explicit RenderTexture(ID3D12Device4& device, TextureInitInfo info);
	~RenderTexture() { release(); }

	void release();

	uint32_t			mipCount() const { return m_mipCount; }
	DescriptorHandle	getRTV(uint32_t mipIndex) const { assert(mipIndex < m_mipCount); return m_rtv[mipIndex]; }
private:

	DescriptorHandle	m_rtv[Texture::MAX_MIPS]{};
	uint32_t			m_mipCount{ 0 };
};