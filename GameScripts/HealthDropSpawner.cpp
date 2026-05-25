#include "pch.h"
#include "HealthDropSpawner.h"

#include "HealthPickup.h"

#include <cmath>
#include <cstdlib>

GameObject* HealthDropSpawner::drop(const char* prefabPath, const Vector3& originPosition, float healAmount, float dropRadius, float dropHeight
)
{
    if (prefabPath == nullptr || prefabPath[0] == '\0')
    {
        Debug::warn("[HealthDropSpawner] Cannot drop health pickup. Prefab path is empty.");
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

    GameObject* pickup = GameObjectAPI::instantiatePrefab(prefabPath, arcOrigin, Vector3::Zero);

    if (pickup == nullptr)
    {
        Debug::warn("[HealthDropSpawner] Failed to instantiate prefab '%s'.", prefabPath);
        return nullptr;
    }

    Script* script = GameObjectAPI::getScript(pickup, "HealthPickup");
    if (script == nullptr)
    {
        Debug::warn("[HealthDropSpawner] Spawned prefab '%s' but it has no HealthPickup script.", prefabPath);

        return pickup;
    }

    HealthPickup* healthPickup = static_cast<HealthPickup*>(script);
    healthPickup->setupDrop(healAmount, landingPosition);

    return pickup;
}