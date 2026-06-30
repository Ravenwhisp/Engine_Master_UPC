#include "pch.h"
#include "BreakableExplosive.h"
#include "Damageable.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(BreakableExplosive, BreakableObject,
    SERIALIZED_FLOAT(m_explosionRadius, "Explosion Radius", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_explosionDamage, "Explosion Damage", 0.0f, 100.0f, 1.0f),
    SERIALIZED_ASSET_REF(m_explosionPrefab, "Explosion Prefab", AssetType::PREFAB)
)

BreakableExplosive::BreakableExplosive(GameObject* owner)
    : BreakableObject(owner)
{
}

void BreakableExplosive::Start()
{
    BreakableObject::Start();
}

void BreakableExplosive::Update()
{
}

void BreakableExplosive::drawGizmo()
{
	DebugDrawAPI::drawCircle(TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner())), 
        Vector3::UnitY,
		Vector3(1.0f, 0.0f, 0.0f),
        m_explosionRadius);
}

void BreakableExplosive::onBreak()
{
	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        Debug::warn("[BreakableExplosive] '%s' has no Transform component.", GameObjectAPI::getName(getOwner()));
        return;
	}

    const Vector3 explosionCenter = TransformAPI::getGlobalPosition(ownerTransform);

    const std::vector<GameObject*> objectsInArea = SceneAPI::getObjectsInCircularArea(Vector2(explosionCenter.x, explosionCenter.z), m_explosionRadius);

    for (GameObject* go : objectsInArea)
    {
		Damageable* damageableScript = GameObjectAPI::findScript<Damageable>(go);
        if (damageableScript == nullptr)
        {
            continue;
        }
        Transform* targetTransform = GameObjectAPI::getTransform(go);
        if (targetTransform == nullptr)
        {
            continue;
        }
        const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);
        const float distanceSquared = Vector3::DistanceSquared(explosionCenter, targetPosition);
        if (distanceSquared > m_explosionRadius * m_explosionRadius)
        {
            continue;
        }
		damageableScript->takeDamage(m_explosionDamage);
	}

    if (m_explosionPrefab.m_ref.isValid())
    {
        GameObjectAPI::instantiatePrefab(m_explosionPrefab.m_ref, TransformAPI::getGlobalPosition(m_brokenObjectTransform), Vector3(0.0f, 0.0f, 0.0f));
    }

    Debug::log("[BreakableExplosive] '%s' exploded dealing %.1f damage in radius %.1f.", GameObjectAPI::getName(getOwner()), m_explosionDamage, m_explosionRadius);

    BreakableObject::breakObject();
}

IMPLEMENT_SCRIPT(BreakableExplosive)