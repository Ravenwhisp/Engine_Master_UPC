#pragma once
#include <cstdint>

enum class AssetType : uint32_t
{
	TEXTURE = 0,
	MODEL = 1,
	MATERIAL = 2,
	MESH = 3,
	FONT = 4,
	PREFAB = 5,
	SCRIPT = 6,

	UNKNOWN = 7
};