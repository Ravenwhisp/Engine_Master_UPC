#pragma once
#include <cstdint>
#include <cstring>

enum class AssetType : uint32_t
{
	TEXTURE = 0,
	MODEL = 1,
	MATERIAL = 2,
	MESH = 3,
	FONT = 4,
	PREFAB = 5,
	ANIMATION = 6,
	SKIN = 7,
	ANIMATION_STATE_MACHINE = 8,
	SCENE = 9,

	UNKNOWN = 10
};

inline const char* AssetTypeToString(uint32_t type)
{
	switch (static_cast<AssetType>(type))
	{
	case AssetType::TEXTURE:                 return "TEXTURE";
	case AssetType::MODEL:                   return "MODEL";
	case AssetType::MATERIAL:                return "MATERIAL";
	case AssetType::MESH:                    return "MESH";
	case AssetType::FONT:                    return "FONT";
	case AssetType::PREFAB:                  return "PREFAB";
	case AssetType::ANIMATION:               return "ANIMATION";
	case AssetType::SKIN:                    return "SKIN";
	case AssetType::ANIMATION_STATE_MACHINE: return "ANIMATION_STATE_MACHINE";
	case AssetType::SCENE:                   return "SCENE";
	default:                                 return "UNKNOWN";
	}
}

inline uint32_t StringToAssetType(const char* s)
{
	if (std::strcmp(s, "TEXTURE") == 0)                 return static_cast<uint32_t>(AssetType::TEXTURE);
	if (std::strcmp(s, "MODEL") == 0)                   return static_cast<uint32_t>(AssetType::MODEL);
	if (std::strcmp(s, "MATERIAL") == 0)                return static_cast<uint32_t>(AssetType::MATERIAL);
	if (std::strcmp(s, "MESH") == 0)                    return static_cast<uint32_t>(AssetType::MESH);
	if (std::strcmp(s, "FONT") == 0)                    return static_cast<uint32_t>(AssetType::FONT);
	if (std::strcmp(s, "PREFAB") == 0)                  return static_cast<uint32_t>(AssetType::PREFAB);
	if (std::strcmp(s, "ANIMATION") == 0)               return static_cast<uint32_t>(AssetType::ANIMATION);
	if (std::strcmp(s, "SKIN") == 0)                    return static_cast<uint32_t>(AssetType::SKIN);
	if (std::strcmp(s, "ANIMATION_STATE_MACHINE") == 0) return static_cast<uint32_t>(AssetType::ANIMATION_STATE_MACHINE);
	if (std::strcmp(s, "SCENE") == 0)                   return static_cast<uint32_t>(AssetType::SCENE);
	return static_cast<uint32_t>(AssetType::UNKNOWN);
}