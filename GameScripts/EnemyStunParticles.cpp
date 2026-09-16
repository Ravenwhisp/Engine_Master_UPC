#include "pch.h"
#include "EnemyStunParticles.h"
#include "ParticleLifecycle.h"

IMPLEMENT_SCRIPT_FIELDS(EnemyStunParticles,
    SERIALIZED_ASSET_REF(m_stunPrefab, "Stun Particle Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(m_heightOffset, "Height Offset", 0.0f, 10.0f, 0.1f)
)

EnemyStunParticles::EnemyStunParticles(GameObject* owner) : Script(owner) {}

void EnemyStunParticles::Start()
{
}

void EnemyStunParticles::OnGameStop()
{
    ParticleLifecycle::destroy(m_stunParticle);
}

void EnemyStunParticles::ensureStunParticle()
{
    Transform* t = GameObjectAPI::getTransform(getOwner());
    Vector3 pos = t ? TransformAPI::getGlobalPosition(t) : Vector3::Zero;
    pos.y += m_heightOffset;
    ParticleLifecycle::ensurePersistent(m_stunParticle, m_stunPrefab.m_id, pos, Vector3::Zero, nullptr);
}

void EnemyStunParticles::startStunParticle()
{
    ensureStunParticle();
    if (!m_stunParticle) return;
    updateStunParticle();
    ParticleLifecycle::activate(m_stunParticle);
}

void EnemyStunParticles::updateStunParticle()
{
    if (!m_stunParticle) return;
    Transform* enemyT    = GameObjectAPI::getTransform(getOwner());
    Transform* particleT = GameObjectAPI::getTransform(m_stunParticle);
    if (enemyT && particleT)
    {
        Vector3 pos = TransformAPI::getGlobalPosition(enemyT);
        pos.y      += m_heightOffset;
        TransformAPI::setGlobalPosition(particleT, pos);
    }
}

void EnemyStunParticles::stopStunParticle()
{
    ParticleLifecycle::deactivate(m_stunParticle);
}

IMPLEMENT_SCRIPT(EnemyStunParticles)
