#include "pch.h"
#include "BreakableHealingDrop.h"

#include "HealthDropSpawner.h"
#include "EnvironmentSound.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(BreakableHealingDrop, BreakableObject,
    SERIALIZED_STRING(m_healthPickupPrefabPath, "Health Pickup Prefab Path"),
    SERIALIZED_FLOAT(m_healthDropAmount, "Health Drop Amount", 0.0f, 100.0f, 1.0f),
    SERIALIZED_FLOAT(m_dropRadius, "Drop Radius", 0.0f, 5.0f, 0.1f),
    SERIALIZED_FLOAT(m_dropHeight, "Drop Height", 0.0f, 5.0f, 0.1f),
    SERIALIZED_INT(m_healthDropQuantity, "Health Drop Quantity")
)

BreakableHealingDrop::BreakableHealingDrop(GameObject* owner)
    : BreakableObject(owner)
{
}

void BreakableHealingDrop::Start()
{
    BreakableObject::Start(); //si no hay nada aqui, quitamos el start
}

void BreakableHealingDrop::Update()
{
}

void BreakableHealingDrop::onBreak()
{
    if (m_healthPickupPrefabPath.empty())
    {
		Debug::warn("[BreakableHealingDrop] '%s' has no health pickup prefab path set.", GameObjectAPI::getName(getOwner()));
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    const Vector3 breakablePosition = TransformAPI::getGlobalPosition(ownerTransform);

    for (int i = 0; i < m_healthDropQuantity; ++i)
    {
        HealthDropSpawner::drop(m_healthPickupPrefabPath.c_str(), breakablePosition, m_healthDropAmount, m_dropRadius, m_dropHeight);
    }

    // It's still a barrel/crate breaking → same break SFX.
    EnvironmentSound::play(getOwner(), "Play_Environment_Barrel_Break");

    BreakableObject::breakObject();
}

IMPLEMENT_SCRIPT(BreakableHealingDrop)