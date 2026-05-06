#pragma once

enum class NavAreaType
{
	Default,
	Spectral,
	Blocked
};

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
	NAV_AREA_DEFAULT,
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