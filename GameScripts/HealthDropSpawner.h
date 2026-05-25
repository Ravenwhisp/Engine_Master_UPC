#pragma once

#include "ScriptAPI.h"

class HealthDropSpawner
{
public:
    static GameObject* drop(const char* prefabPath, const Vector3& originPosition, float healAmount, float dropRadius, float dropHeight);
};