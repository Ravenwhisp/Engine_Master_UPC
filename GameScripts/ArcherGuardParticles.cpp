#include "pch.h"
#include "ArcherGuardParticles.h"
#include "ParticleLifecycle.h"

IMPLEMENT_SCRIPT_FIELDS(ArcherGuardParticles,
    SERIALIZED_ASSET_REF(m_trailPrefab, "Trail Particle Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_barrageFloorPath, "Barrage Floor Prefab Path"),
    SERIALIZED_ASSET_REF(m_barrageFloorPrefab, "Barrage Floor Particle Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_arrowSparksPath, "Arrow Sparks Prefab Path"),
    SERIALIZED_ASSET_REF(m_arrowSparksPrefab, "Arrow Sparks Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_barrageImpactPrefab, "Barrage Impact Particle Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_somersaultPrefab, "Somersault Particle Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(m_barrageFloorYOffset, "Barrage Floor Y Offset", -5.0f, 5.0f, 0.5f),
    SERIALIZED_FLOAT(m_barrageImpactYOffset, "Barrage Impact Y Offset", -5.0f, 5.0f, 0.5f),
    SERIALIZED_FLOAT(m_barrageImpactLifetime, "Barrage Impact Lifetime", 0.0f, 10.0f, 0.1f)
)

ArcherGuardParticles::ArcherGuardParticles(GameObject* owner)
    : Script(owner)
{
}

void ArcherGuardParticles::Start()
{
}

void ArcherGuardParticles::OnGameStop()
{
    ParticleLifecycle::destroy(m_trailGO);
    ParticleLifecycle::destroy(m_arrowSparksGO);
    ParticleLifecycle::destroy(m_barrageFloorParticle);
    ParticleLifecycle::destroy(m_barrageImpactParticle);
    ParticleLifecycle::destroy(m_somersaultParticle);
    m_somersaultParticleTransform = nullptr;
}

void ArcherGuardParticles::Update()
{
    if (!m_barrageImpactParticle)
    {
        return;
    }

    m_barrageImpactTimer -= Time::getDeltaTime();

    if (m_barrageImpactTimer <= 0.0f)
    {
        GameObjectAPI::removeGameObject(m_barrageImpactParticle);
        m_barrageImpactParticle = nullptr;
        m_barrageImpactTimer = 0.0f;
    }
}

void ArcherGuardParticles::ensureTrailParticle(const Vector3& pos)
{
    ParticleLifecycle::ensurePersistent(m_trailGO, m_trailPrefab.m_id, pos, Vector3::Zero, nullptr);
}

void ArcherGuardParticles::ensureArrowSparksParticle(const Vector3& pos)
{
    ParticleLifecycle::ensurePersistent(m_arrowSparksGO, m_arrowSparksPrefab.m_id, pos, Vector3::Zero, nullptr);
}

void ArcherGuardParticles::ensureBarrageFloorParticle(const Vector3& position)
{
    ParticleLifecycle::ensurePersistent(m_barrageFloorParticle, m_barrageFloorPrefab.m_id, position, Vector3::Zero, nullptr);
}

void ArcherGuardParticles::ensureSomersaultParticle()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    const Vector3 position = ownerTransform ? TransformAPI::getGlobalPosition(ownerTransform) : Vector3::Zero;
    const Vector3 rotation = ownerTransform ? TransformAPI::getGlobalEulerDegrees(ownerTransform) : Vector3::Zero;

    ParticleLifecycle::ensurePersistent(m_somersaultParticle, m_somersaultPrefab.m_id, position, rotation, nullptr);

    if (m_somersaultParticle)
    {
        m_somersaultParticleTransform = GameObjectAPI::getTransform(m_somersaultParticle);
    }
}

// ── Basic attack trail ────────────────────────────────────────────────────────

void ArcherGuardParticles::spawnBasicAttackTrail(const Vector3& pos)
{
    ensureTrailParticle(pos);

    if (!m_trailGO)
    {
        return;
    }

    syncBasicAttackTrail(pos, Vector3::Zero);
    ParticleLifecycle::activate(m_trailGO);
}

void ArcherGuardParticles::syncBasicAttackTrail(const Vector3& pos, const Vector3& eulerDeg)
{
    if (!m_trailGO)
    {
        return;
    }

    Transform* trailTransform = GameObjectAPI::getTransform(m_trailGO);

    if (!trailTransform)
    {
        return;
    }

    TransformAPI::setGlobalPosition(trailTransform, pos);
    TransformAPI::setGlobalRotationEuler(trailTransform, eulerDeg);
}

void ArcherGuardParticles::stopBasicAttackTrail()
{
    ParticleLifecycle::deactivate(m_trailGO);
}

// ── Basic attack arrow sparks ─────────────────────────────────────────────────

void ArcherGuardParticles::spawnArrowSparks(const Vector3& pos)
{
    ensureArrowSparksParticle(pos);

    if (!m_arrowSparksGO)
    {
        return;
    }

    syncArrowSparks(pos, Vector3::Zero);
    ParticleLifecycle::activate(m_arrowSparksGO);
}

void ArcherGuardParticles::syncArrowSparks(const Vector3& pos, const Vector3& eulerDeg)
{
    if (!m_arrowSparksGO)
    {
        return;
    }

    Transform* sparksTransform = GameObjectAPI::getTransform(m_arrowSparksGO);

    if (!sparksTransform)
    {
        return;
    }

    TransformAPI::setGlobalPosition(sparksTransform, pos);
    TransformAPI::setGlobalRotationEuler(sparksTransform, eulerDeg);
}

void ArcherGuardParticles::stopArrowSparks()
{
    ParticleLifecycle::deactivate(m_arrowSparksGO);
}

void ArcherGuardParticles::startBarrageFloorParticle(const Vector3& position)
{
    Vector3 particlePosition = position;
    particlePosition.y += m_barrageFloorYOffset;

    ensureBarrageFloorParticle(particlePosition);

    if (!m_barrageFloorParticle)
    {
        Debug::warn("[ArcherGuardParticles] Barrage floor particle prefab is missing.");
        return;
    }

    Transform* floorTransform = GameObjectAPI::getTransform(m_barrageFloorParticle);
    if (floorTransform)
    {
        TransformAPI::setGlobalPosition(floorTransform, particlePosition);
    }

    ParticleLifecycle::activate(m_barrageFloorParticle);
}

void ArcherGuardParticles::stopBarrageFloorParticle()
{
    ParticleLifecycle::deactivate(m_barrageFloorParticle);
}

void ArcherGuardParticles::playBarrageImpactParticle(const Vector3& position)
{
    if (!m_barrageImpactPrefab.m_id.isValid())
    {
        Debug::warn("[ArcherGuardParticles] Barrage impact particle prefab is missing.");
        return;
    }

    if (m_barrageImpactParticle)
    {
        GameObjectAPI::removeGameObject(m_barrageImpactParticle);
        m_barrageImpactParticle = nullptr;
    }

    Vector3 particlePosition = position;
    particlePosition.y += m_barrageImpactYOffset;

    m_barrageImpactParticle = GameObjectAPI::instantiatePrefab(m_barrageImpactPrefab.m_id, particlePosition, Vector3::Zero);

    if (!m_barrageImpactParticle)
    {
        m_barrageImpactTimer = 0.0f;
        return;
    }

    m_barrageImpactTimer = m_barrageImpactLifetime > 0.0f
        ? m_barrageImpactLifetime
        : ParticleLifecycle::kDefaultOneShotLifetime;
}

void ArcherGuardParticles::startChargeParticle()
{
    ensureSomersaultParticle();

    if (!m_somersaultParticle)
    {
        Debug::warn("[ArcherGuardParticles] Somersault particle prefab is missing.");
        return;
    }

    updateChargeParticle();
    ParticleLifecycle::activate(m_somersaultParticle);
}

void ArcherGuardParticles::updateChargeParticle()
{
    if (!m_somersaultParticleTransform)
    {
        return;
    }

    Transform* archerTransform = GameObjectAPI::getTransform(getOwner());

    if (!archerTransform)
    {
        return;
    }

    TransformAPI::setGlobalPosition(m_somersaultParticleTransform, TransformAPI::getGlobalPosition(archerTransform));
    TransformAPI::setGlobalRotationEuler(m_somersaultParticleTransform, TransformAPI::getGlobalEulerDegrees(archerTransform));
}

void ArcherGuardParticles::stopChargeParticle()
{
    ParticleLifecycle::deactivate(m_somersaultParticle);
}

IMPLEMENT_SCRIPT(ArcherGuardParticles)
