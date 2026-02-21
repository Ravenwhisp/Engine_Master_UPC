#pragma once
#include "Globals.h"

struct Vertex
{
	Vector3 position;
	Vector3 normal = Vector3::UnitZ;
	Vector3 tangent = Vector3::UnitX;
	Vector2 texCoord0;
};