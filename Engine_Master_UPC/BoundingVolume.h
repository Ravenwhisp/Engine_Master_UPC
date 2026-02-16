#pragma once
#include "Globals.h"
#include "Frustum.h"

class BoundingVolume
{
public:
	virtual bool test(const Engine::Frustum& frustum) const = 0;
	virtual void render() = 0;
};