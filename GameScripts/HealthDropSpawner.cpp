#include "pch.h"
#include "HealthDropSpawner.h"

#include "HealthPickup.h"

#include <cmath>
#include <cstdlib>

GameObject* HealthDropSpawner::drop(const AssetReference& prefabRef, const Vector3& originPosition, float healAmount, float dropRadius, float dropHeight
)
{
    if (!prefabRef.isValid())
    {
        Debug::warn("[HealthDropSpawner] Cannot drop health pickup. Prefab reference is invalid.");
        return nullptr;
    }

    const float angle = (static_cast<float>(rand()) / RAND_MAX) * 6.283185f;
    const float distance = (static_cast<float>(rand()) / RAND_MAX) * dropRadius;

    Vector3 offset;
    offset.x = std::cos(angle) * distance;
    offset.y = 0.0f;
    offset.z = std::sin(angle) * distance;

    const Vector3 landingPosition = originPosition + offset;

    const Vector3 arcOrigin = Vector3(originPosition.x, originPosition.y + dropHeight, originPosition.z);

    GameObject* pickup = GameObjectAPI::instantiatePrefab(prefabRef, arcOrigin, Vector3::Zero);

    if (pickup == nullptr)
    {
        Debug::warn("[HealthDropSpawner] Failed to instantiate prefab (UID %llu).", prefabRef.m_uid);
        return nullptr;
    }

    HealthPickup* healthPickup = GameObjectAPI::findScript<HealthPickup>(pickup);
    if (healthPickup == nullptr)
    {
        Debug::warn("[HealthDropSpawner] Spawned prefab (UID %llu) but it has no HealthPickup script.", prefabRef.m_uid);

        return pickup;
    }

    healthPickup->setupDrop(healAmount, landingPosition);

    return pickup;
}