#pragma once

#include "ScriptAPI.h"
struct AssetId;

class HealthDropSpawner
{
public:
    static GameObject* drop(const AssetId& prefabRef, const Vector3& originPosition, float healAmount, float dropRadius, float dropHeight);
};