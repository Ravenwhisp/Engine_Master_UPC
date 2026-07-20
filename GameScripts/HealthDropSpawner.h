#pragma once

#include "ScriptAPI.h"

class HealthDropSpawner
{
public:
    static GameObject* drop(const AssetId& prefabRef, const Vector3& originPosition, float healAmount, float dropRadius, float dropHeight);
};