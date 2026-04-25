#pragma once

#include <cstdint>
#include <vector>

struct TextureImage
{
	friend class cereal::access;

	uint32_t slicePitch = 0;
	uint32_t rowPitch = 0;
	std::vector<uint8_t> pixels;

	template<class Archive>
	void serialize(Archive& ar)
	{
		ar(rowPitch, slicePitch, pixels);
	}
};