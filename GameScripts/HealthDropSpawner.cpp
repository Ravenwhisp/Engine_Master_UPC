#include "pch.h"
#include "HealthDropSpawner.h"

#include "HealthPickup.h"


GameObject* HealthDropSpawner::drop(const AssetId& prefabRef, const Vector3& originPosition, float healAmount, float dropRadius, float dropHeight
)
{
    if (!prefabRef.isValid())
    {
        Debug::warn("[HealthDropSpawner] Cannot drop health pickup. Prefab ref is invalid.");
        return nullptr;
    }

    Vector3 landingPosition = originPosition;

    Vector3 sampled;
    const Vector3 searchExtents(2.0f, 2.0f, 2.0f);
    const int kMaxAttempts = 8;

    if (NavigationAPI::findRandomReachablePointAround(
            originPosition, dropRadius, sampled, searchExtents, kMaxAttempts))
    {
        landingPosition = Vector3(sampled.x, originPosition.y, sampled.z);
    }

    const Vector3 arcOrigin = Vector3(originPosition.x, originPosition.y + dropHeight, originPosition.z);

    GameObject* pickup = GameObjectAPI::instantiatePrefab(prefabRef, arcOrigin, Vector3::Zero);

    if (pickup == nullptr)
    {
        Debug::warn("[HealthDropSpawner] Failed to instantiate prefab.");
        return nullptr;
    }

    Script* script = GameObjectAPI::getScript(pickup, "HealthPickup");
    if (script == nullptr)
    {
        Debug::warn("[HealthDropSpawner] Spawned prefab but it has no HealthPickup script.");

        return pickup;
    }

    HealthPickup* healthPickup = static_cast<HealthPickup*>(script);
    healthPickup->setupDrop(healAmount, landingPosition);

    return pickup;
}