#include "pch.h"
#include "EnemyStunParticles.h"

IMPLEMENT_SCRIPT_FIELDS(EnemyStunParticles,
    SERIALIZED_STRING(m_stunPrefab, "Stun Particle Prefab"),
    SERIALIZED_FLOAT(m_heightOffset, "Height Offset", 0.0f, 10.0f, 0.1f)
)

EnemyStunParticles::EnemyStunParticles(GameObject* owner) : Script(owner) {}

void EnemyStunParticles::Start() {}

void EnemyStunParticles::startStunParticle()
{
    stopStunParticle();
    if (m_stunPrefab.empty()) return;
    Transform* t = GameObjectAPI::getTransform(getOwner());
    Vector3 pos  = t ? TransformAPI::getGlobalPosition(t) : Vector3::Zero;
    pos.y       += m_heightOffset;
    m_stunParticle = GameObjectAPI::instantiatePrefab(m_stunPrefab.c_str(), pos, Vector3::Zero);
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
    if (m_stunParticle) { GameObjectAPI::removeGameObject(m_stunParticle); m_stunParticle = nullptr; }
}

IMPLEMENT_SCRIPT(EnemyStunParticles)
