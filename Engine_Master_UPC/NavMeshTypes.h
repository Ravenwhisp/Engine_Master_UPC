#pragma once
#include <cstring>
#include "SimpleMath.h"

using DirectX::SimpleMath::Vector3;

enum class NavAreaType
{
	Default,
	Spectral,
	Blocked,
	DashGap
};

inline const char* NavAreaTypeToString(uint32_t v)
{
    switch (static_cast<NavAreaType>(v))
    {
    case NavAreaType::Default:  return "Default";
    case NavAreaType::Spectral: return "Spectral";
	case NavAreaType::Blocked:  return "Blocked";
	case NavAreaType::DashGap: return "DashGap";
    default: return "Default";
    }
}

inline uint32_t StringToNavAreaType(const char* s)
{
    if (std::strcmp(s, "Default") == 0)  return 0;
    if (std::strcmp(s, "Spectral") == 0) return 1;
    if (std::strcmp(s, "Blocked") == 0)  return 2;
	if (std::strcmp(s, "DashGap") == 0) return 3;
    return 0;
}

enum class NavPolyFlags
{
	Default = 1 << 0,
	Spectral = 1 << 1,
	DashGap = 1 << 2
};

enum class NavAgentProfile
{
	PlayerNormal,
	PlayerSpectral,
	PlayerDash,
	EnemyGround
};

enum class NavAreaId
{
	NAV_AREA_DEFAULT = 1,
	NAV_AREA_SPECTRAL,
	NAV_AREA_DASHGAP
};


struct NavModifierVolumeData
{
	Vector3 position = Vector3::Zero;
	Vector3 halfExtents = Vector3::One;
	NavAreaType areaType = NavAreaType::Default;
	bool enabled = true;
	int priority = 0;
};