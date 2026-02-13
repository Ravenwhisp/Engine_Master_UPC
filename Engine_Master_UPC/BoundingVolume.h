#pragma once
#include "Globals.h"
#include "Frustum.h"

class BoundingVolume
{
public:
	virtual bool test(const Frustum& frustum) = 0;
	virtual void render() = 0;
};