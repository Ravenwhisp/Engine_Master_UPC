#pragma once
#include "Texture.h"

class DepthBuffer : public Texture {
public:
	DepthBuffer() = default;
	explicit DepthBuffer(ID3D12Device4& device, TextureInitInfo info);
	~DepthBuffer() { release(); }

	void				release();
	DescriptorHandle	getDSV() { return m_dsv; }
private:
	DescriptorHandle m_dsv{};
};