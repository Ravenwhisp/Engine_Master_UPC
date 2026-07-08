#include "pch.h"
#include "ArcherGuardParticles.h"
#include "ArcherArrowProjectile.h"

IMPLEMENT_SCRIPT_FIELDS(ArcherGuardParticles,
    SERIALIZED_ASSET_REF(m_trailPrefab,  "Trail Particle Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_volleyPrefab, "Volley Particle Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_chargePrefab, "Charge Particle Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_arrowPrefab,  "Barrage Arrow Prefab", AssetType::PREFAB)
)

ArcherGuardParticles::ArcherGuardParticles(GameObject* owner) : Script(owner) {}

void ArcherGuardParticles::Start() {}

// ── Basic attack trail ────────────────────────────────────────────────────────

void ArcherGuardParticles::spawnBasicAttackTrail(const Vector3& pos)
{
    stopBasicAttackTrail();
    if (!m_arrowPrefab.m_ref.isValid()) return;
    m_trailGO = GameObjectAPI::instantiatePrefab(m_trailPrefab.m_ref, pos, Vector3::Zero);
}

void ArcherGuardParticles::syncBasicAttackTrail(const Vector3& pos, const Vector3& eulerDeg)
{
    if (!m_trailGO) return;
    Transform* t = GameObjectAPI::getTransform(m_trailGO);
    if (t)
    {
        TransformAPI::setGlobalPosition(t, pos);
        TransformAPI::setGlobalRotationEuler(t, eulerDeg);
    }
}

void ArcherGuardParticles::stopBasicAttackTrail()
{
    if (m_trailGO) { GameObjectAPI::removeGameObject(m_trailGO); m_trailGO = nullptr; }
}

// ── Barrage ───────────────────────────────────────────────────────────────────

static const Vector3 k_barrageOffsets[] = {
    Vector3( 0.0f, 0.0f,  0.0f),
    Vector3( 1.2f, 0.0f,  0.0f),
    Vector3(-1.2f, 0.0f,  0.0f),
    Vector3( 0.0f, 0.0f,  1.2f),
    Vector3( 0.0f, 0.0f, -1.2f),
};
static const int   k_barrageArrowCount  = 5;
static const float k_barrageSpawnHeight = 8.0f;

void ArcherGuardParticles::spawnBarrageArrows(const Vector3& impactPos, float landDelay)
{
    stopBarrageArrows();
    if (!m_arrowPrefab.m_ref.isValid()) return;

    const float speed = k_barrageSpawnHeight / (landDelay > 0.0f ? landDelay : 1.0f);

    for (int i = 0; i < k_barrageArrowCount; ++i)
    {
        Vector3 target   = impactPos + k_barrageOffsets[i];
        Vector3 spawnPos = target;
        spawnPos.y      += k_barrageSpawnHeight;

        GameObject* go = GameObjectAPI::instantiatePrefab(m_arrowPrefab.m_ref, spawnPos, Vector3::Zero);
        if (go)
        {
            ArcherArrowProjectile* arrow = GameObjectAPI::findScript<ArcherArrowProjectile>(go);
            if (arrow) arrow->launch(spawnPos, target, speed);
            m_barrageArrows.push_back(go);
        }
    }
}

void ArcherGuardParticles::spawnImpactParticle(const Vector3& impactPos)
{
    stopBarrageArrows();
    if (m_arrowPrefab.m_ref.isValid())
        GameObjectAPI::instantiatePrefab(m_volleyPrefab.m_ref, impactPos, Vector3::Zero);
}

void ArcherGuardParticles::stopBarrageArrows()
{
    for (GameObject* go : m_barrageArrows)
        if (go) GameObjectAPI::removeGameObject(go);
    m_barrageArrows.clear();
}

// ── Charge / somersault ───────────────────────────────────────────────────────

void ArcherGuardParticles::startChargeParticle()
{
    stopChargeParticle();
    if (!m_arrowPrefab.m_ref.isValid()) return;
    Transform* t = GameObjectAPI::getTransform(getOwner());
    Vector3 pos  = t ? TransformAPI::getGlobalPosition(t) : Vector3::Zero;
    m_chargeParticleGO = GameObjectAPI::instantiatePrefab(m_chargePrefab.m_ref, pos, Vector3::Zero);
}

void ArcherGuardParticles::updateChargeParticle()
{
    if (!m_chargeParticleGO) return;
    Transform* archerT   = GameObjectAPI::getTransform(getOwner());
    Transform* particleT = GameObjectAPI::getTransform(m_chargeParticleGO);
    if (archerT && particleT)
        TransformAPI::setGlobalPosition(particleT, TransformAPI::getGlobalPosition(archerT));
}

void ArcherGuardParticles::stopChargeParticle()
{
    if (m_chargeParticleGO) { GameObjectAPI::removeGameObject(m_chargeParticleGO); m_chargeParticleGO = nullptr; }
}

IMPLEMENT_SCRIPT(ArcherGuardParticles)
