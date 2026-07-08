#pragma once
#include <cstdint>
#include <cstring>

#define ASSET_TYPE_LIST(X) \
    X(TEXTURE)                 \
    X(MODEL)                   \
    X(MATERIAL)                \
    X(MESH)                    \
    X(FONT)                    \
    X(PREFAB)                  \
    X(ANIMATION)               \
    X(SKIN)                    \
    X(ANIMATION_STATE_MACHINE) \
    X(SCENE)                   \
    X(NAVMESH)                 \
    X(SOUND_BANK)              \
    X(DATA_CONTAINER) 			\
    X(UNKNOWN)

#define ASSET_ENUM(name) name,
#define ASSET_SWITCH(name) case AssetType::name: return #name;
#define ASSET_IF(name) if (std::strcmp(s, #name) == 0) return static_cast<uint32_t>(AssetType::name);

enum class AssetType : uint32_t
{
	ASSET_TYPE_LIST(ASSET_ENUM)
};

inline const char* AssetTypeToString(uint32_t type)
{
	switch (static_cast<AssetType>(type))
	{
		ASSET_TYPE_LIST(ASSET_SWITCH)
	default:
		return "UNKNOWN";
	}
}

inline uint32_t StringToAssetType(const char* s)
{
	ASSET_TYPE_LIST(ASSET_IF)
	return static_cast<uint32_t>(AssetType::UNKNOWN);
}
