#include "pch.h"
#include "SkeletonParticles.h"

IMPLEMENT_SCRIPT_FIELDS(SkeletonParticles,
    SERIALIZED_STRING(m_reviveParticlePath, "Revive Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_reviveParticlePrefab, "Revive Particle Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_shieldHitPath, "Shield Hit Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_shieldHitPrefab, "Shield Hit Particle Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_thirdAttackHitPath, "Third Attack Hit Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_thirdAttackHitPrefab, "Third Attack Hit Particle Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(m_reviveYOffset, "Revive Y Offset", -5.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_reviveForwardOffset, "Revive Forward Offset", -5.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_reviveDeactivateDelay, "Revive Deactivate Delay", 0.0f, 10.0f, 0.1f)
)

SkeletonParticles::SkeletonParticles(GameObject* owner)
    : Script(owner)
{
}

void SkeletonParticles::Start()
{
    m_ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (m_ownerTransform == nullptr)
    {
        Debug::warn("[SkeletonParticles] Owner transform not found on '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

void SkeletonParticles::OnGameStop()
{
    m_timedParticles.clear();
    ParticleLifecycle::destroy(m_reviveParticle);
    m_reviveParticleTransform = nullptr;
}

void SkeletonParticles::Update()
{
    m_timedParticles.update(Time::getDeltaTime());

    if (m_reviveParticle != nullptr && GameObjectAPI::isActiveSelf(m_reviveParticle))
    {
        updateReviveParticle();
    }
}

void SkeletonParticles::ensureReviveParticle()
{
    if (m_ownerTransform == nullptr)
    {
        m_ownerTransform = GameObjectAPI::getTransform(getOwner());
    }

    ParticleLifecycle::ensurePersistent(
        m_reviveParticle,
        m_reviveParticlePrefab.m_id,
        getReviveParticlePosition(),
        getOwnerRotation(),
        nullptr
    );

    if (m_reviveParticle != nullptr)
    {
        m_reviveParticleTransform = GameObjectAPI::getTransform(m_reviveParticle);
    }
}

void SkeletonParticles::startReviveParticle()
{
    if (!m_reviveParticlePrefab.m_id.isValid())
    {
        Debug::warn("[SkeletonParticles] Revive particle prefab is missing on '%s'.", GameObjectAPI::getName(getOwner()));
        return;
    }

    ensureReviveParticle();

    if (m_reviveParticle == nullptr)
    {
        Debug::warn("[SkeletonParticles] Could not instantiate revive particle on '%s'.", GameObjectAPI::getName(getOwner()));
        return;
    }

    updateReviveParticle();
    ParticleLifecycle::activateTimed(m_timedParticles, m_reviveParticle, m_reviveDeactivateDelay);
}

void SkeletonParticles::stopReviveParticle()
{
    ParticleLifecycle::deactivate(m_reviveParticle);
}

void SkeletonParticles::updateReviveParticle()
{
    if (m_reviveParticleTransform == nullptr)
    {
        return;
    }

    TransformAPI::setGlobalPosition(m_reviveParticleTransform, getReviveParticlePosition());
    TransformAPI::setGlobalRotationEuler(m_reviveParticleTransform, getOwnerRotation());
}

Vector3 SkeletonParticles::getReviveParticlePosition() const
{
    if (m_ownerTransform == nullptr)
    {
        return Vector3::Zero;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(m_ownerTransform);
    const Vector3 ownerForward = TransformAPI::getForward(m_ownerTransform);

    return Vector3(
        ownerPosition.x + ownerForward.x * m_reviveForwardOffset,
        ownerPosition.y + m_reviveYOffset,
        ownerPosition.z + ownerForward.z * m_reviveForwardOffset
    );
}

Vector3 SkeletonParticles::getOwnerRotation() const
{
    if (m_ownerTransform == nullptr)
    {
        return Vector3::Zero;
    }

    return TransformAPI::getGlobalEulerDegrees(m_ownerTransform);
}

IMPLEMENT_SCRIPT(SkeletonParticles)
