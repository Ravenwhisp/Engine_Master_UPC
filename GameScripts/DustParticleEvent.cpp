#include "pch.h"
#include "DustParticleEvent.h"

#include "GameplayEventTrigger.h"

IMPLEMENT_SCRIPT_FIELDS(DustParticleEvent,
    SERIALIZED_ASSET_REF(m_dustPrefab, "Dust Prefab", AssetType::PREFAB),
    SERIALIZED_COMPONENT_REF_VECTOR(m_spawnPoints, "Spawn Points", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_lifetime, "Lifetime", 0.5f, 10.0f, 0.25f)
)

DustParticleEvent::DustParticleEvent(GameObject* owner)
    : GameplayEventAction(owner)
{
}

void DustParticleEvent::Update()
{
    m_timedParticles.update(Time::getDeltaTime());
}

void DustParticleEvent::OnGameStop()
{
    m_timedParticles.clear();
}

void DustParticleEvent::executeEvent(GameplayEventTrigger* trigger)
{
    if (m_spawnPoints.empty())
    {
        Debug::warn(
            "DustParticleEvent on '%s' has no spawn points assigned.",
            GameObjectAPI::getName(getOwner())
        );
        return;
    }

    for (ComponentRef<Transform>& spawnPointRef : m_spawnPoints)
    {
        Transform* spawnPoint = spawnPointRef.getReferencedComponent();

        if (spawnPoint == nullptr)
        {
            continue;
        }

        const Vector3 position =
            TransformAPI::getGlobalPosition(spawnPoint);

        ParticleLifecycle::spawnOneShotTimed(
            m_timedParticles,
            m_dustPrefab.m_id,
            position,
            Vector3::Zero,
            m_lifetime
        );
    }
}

IMPLEMENT_SCRIPT(DustParticleEvent)