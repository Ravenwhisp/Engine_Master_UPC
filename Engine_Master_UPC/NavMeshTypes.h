#pragma once

enum class NavAreaType
{
	Default,
	Spectral,
	Blocked
};

enum class NavPolyFlags
{
	Default,
	Spectral
};

enum class NavAgentProfile
{
	PlayerNormal,
	PlayerSpectral,
	EnemyGround
};

struct NavModifierVolumeData
{
	Vector3 position = Vector3::Zero;
	Vector3 halfExtents = Vector3::One;
	NavAreaType areaType = NavAreaType::Default;
	bool enabled = true;
	int priority = 0;
};