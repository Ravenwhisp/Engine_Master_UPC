#pragma once
#include "Globals.h"

struct Vertex
{
	Vector3 position;
	Vector2 texCoord0;
	Vector3 normal = Vector3::UnitZ;

	uint16_t joints[4] = { 0, 0, 0, 0 };
	Vector4  weights = Vector4::Zero;
};