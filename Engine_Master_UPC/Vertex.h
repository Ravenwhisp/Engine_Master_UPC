#pragma once
#include "Globals.h"

struct Vertex
{
	Vector3 position;
	Vector2 texCoord0;
	Vector3 normal = Vector3::UnitZ;
};