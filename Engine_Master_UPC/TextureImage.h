#pragma once

#include <cstdint>
#include <vector>
struct TextureImage
{
	uint32_t slicePitch = 0;
	uint32_t rowPitch = 0;
	std::vector<uint8_t> pixels;
};