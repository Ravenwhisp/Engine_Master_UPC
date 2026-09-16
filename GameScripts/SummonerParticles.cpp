#include "pch.h"
#include "SummonerParticles.h"

IMPLEMENT_SCRIPT_FIELDS(SummonerParticles,
    SERIALIZED_STRING(m_summonParticlePath, "Summon Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_summonParticlePrefab, "Summon Particle Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_teleportParticlePath, "Teleport Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_teleportParticlePrefab, "Teleport Particle Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(m_summonParticleLifetime, "Summon Particle Lifetime", 0.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_teleportDeactivateDelay, "Teleport Deactivate Delay", 0.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_summonYOffset, "Summon Particle Y Offset", -5.0f, 5.0f, 0.05f)
)

SummonerParticles::SummonerParticles(GameObject* owner)
    : Script(owner)
{
}

void SummonerParticles::OnGameStop()
{
    m_timedParticles.clear();
}

void SummonerParticles::Update()
{
    m_timedParticles.update(Time::getDeltaTime());
}

void SummonerParticles::playSummonParticle(const Vector3& position)
{
    spawnSummonParticle(position);
}

void SummonerParticles::playTeleportParticle(const Vector3& position)
{
    spawnTeleportBurst(position);
}

void SummonerParticles::spawnSummonParticle(const Vector3& position)
{
    if (!m_summonParticlePrefab.m_id.isValid())
    {
        Debug::warn("[SummonerParticles] Summon particle prefab is missing on '%s'.", GameObjectAPI::getName(getOwner()));
        return;
    }

    Vector3 spawnPosition = position;
    spawnPosition.y += m_summonYOffset;

    ParticleLifecycle::spawnOneShotTimed(
        m_timedParticles,
        m_summonParticlePrefab.m_id,
        spawnPosition,
        Vector3::Zero,
        m_summonParticleLifetime
    );
}

void SummonerParticles::spawnTeleportBurst(const Vector3& position)
{
    if (!m_teleportParticlePrefab.m_id.isValid())
    {
        Debug::warn("[SummonerParticles] Teleport particle prefab is missing on '%s'.", GameObjectAPI::getName(getOwner()));
        return;
    }

    GameObject* teleportInstance = ParticleLifecycle::spawnOneShot(
        m_teleportParticlePrefab.m_id,
        position,
        getOwnerRotation()
    );

    if (teleportInstance == nullptr)
    {
        Debug::warn("[SummonerParticles] Could not instantiate teleport particle on '%s'.", GameObjectAPI::getName(getOwner()));
        return;
    }

    ParticleLifecycle::disableSelfDestruct(teleportInstance);
    ParticleLifecycle::activateTimed(m_timedParticles, teleportInstance, m_teleportDeactivateDelay);
    m_timedParticles.scheduleDestroy(teleportInstance, m_teleportDeactivateDelay + 0.05f);
}

Vector3 SummonerParticles::getOwnerRotation() const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (ownerTransform == nullptr)
    {
        return Vector3::Zero;
    }

    return TransformAPI::getGlobalEulerDegrees(ownerTransform);
}

IMPLEMENT_SCRIPT(SummonerParticles)
