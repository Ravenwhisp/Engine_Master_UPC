#pragma once
#include <cstring>

enum class NavAreaType
{
	Default,
	Spectral,
	Blocked
};

inline const char* NavAreaTypeToString(uint32_t v)
{
    switch (static_cast<NavAreaType>(v))
    {
    case NavAreaType::Default:  return "Default";
    case NavAreaType::Spectral: return "Spectral";
    case NavAreaType::Blocked:  return "Blocked";
    default: return "Default";
    }
}

inline uint32_t StringToNavAreaType(const char* s)
{
    if (std::strcmp(s, "Default") == 0)  return 0;
    if (std::strcmp(s, "Spectral") == 0) return 1;
    if (std::strcmp(s, "Blocked") == 0)  return 2;
    return 0;
}

enum class NavPolyFlags
{
	Default = 1 << 0,
	Spectral = 1 << 1
};

enum class NavAgentProfile
{
	PlayerNormal,
	PlayerSpectral,
	EnemyGround
};

enum class NavAreaId
{
	NAV_AREA_DEFAULT = 1,
	NAV_AREA_SPECTRAL
};


struct NavModifierVolumeData
{
	Vector3 position = Vector3::Zero;
	Vector3 halfExtents = Vector3::One;
	NavAreaType areaType = NavAreaType::Default;
	bool enabled = true;
	int priority = 0;
};