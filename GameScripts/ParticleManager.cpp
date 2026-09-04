#include "pch.h"
#include "ParticleManager.h"

IMPLEMENT_SCRIPT_FIELDS(ParticleManager,
    SERIALIZED_COMPONENT_REF(m_playerTransform, "Player Transform", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_activationDistance, "Activation Distance", 0.0f, 500.0f, 1.0f),
    SERIALIZED_FLOAT(m_checkIntervalSeconds, "Check Interval (s)", 0.1f, 10.0f, 0.1f)
)

ParticleManager::ParticleManager(GameObject* owner)
    : Script(owner)
{
}

void ParticleManager::Start()
{
    m_timer = 0.0f;
    refreshParticleCache();
    Debug::log("[ParticleManager] Cached %zu initially-active particle objects, distance=%.1f, interval=%.2fs.",
        m_managedParticles.size(), m_activationDistance, m_checkIntervalSeconds);
}

void ParticleManager::Update()
{
    m_timer += Time::getDeltaTime();
    if (m_timer >= m_checkIntervalSeconds)
    {
        m_timer -= m_checkIntervalSeconds;
        updateActivity();
    }
}

void ParticleManager::refreshParticleCache()
{
    std::vector<GameObject*> all = SceneAPI::findAllGameObjectsByComponent(
        ComponentType::PARTICLE_SYSTEM, false);

    GameObject* self = getOwner();

    for (GameObject* obj : all)
    {
        if (!obj) continue;
        if (obj == self) continue;
        if (!GameObjectAPI::isActiveSelf(obj)) continue;

        ManagedParticle entry;
        entry.gameObject = obj;
        m_managedParticles.push_back(entry);
    }
}

void ParticleManager::updateActivity()
{
    Transform* playerT = m_playerTransform.getReferencedComponent();
    if (!playerT) return;

    const Vector3 playerPos = TransformAPI::getGlobalPosition(playerT);

    for (ManagedParticle& mp : m_managedParticles)
    {
        GameObject* obj = mp.gameObject;
        if (!obj) continue;

        Transform* t = GameObjectAPI::getTransform(obj);
        if (!t) continue;

        const float distance       = Vector3::Distance(playerPos, TransformAPI::getGlobalPosition(t));
        const bool  withinRange    = (distance <= m_activationDistance);
        const bool  isActive       = GameObjectAPI::isActiveSelf(obj);

        if (isActive && !withinRange)
        {
            ParticleSystemComponent* ps = ParticleSystemAPI::getParticleSystemComponent(obj);
            if (ps)
                ParticleSystemAPI::reset(ps);

            GameObjectAPI::setActive(obj, false);
            mp.deactivatedByManager = true;
        }
        else if (!isActive && withinRange && mp.deactivatedByManager)
        {
            GameObjectAPI::setActive(obj, true);
            mp.deactivatedByManager = false;
        }
        else if (isActive && withinRange)
        {
            mp.deactivatedByManager = false;
        }
    }
}

void ParticleManager::drawGizmo()
{
    Transform* playerT = m_playerTransform.getReferencedComponent();
    if (!playerT) return;

    const Vector3 playerPos = TransformAPI::getGlobalPosition(playerT);
    DebugDrawAPI::drawSphere(playerPos, Vector3(0.0f, 1.0f, 0.5f), m_activationDistance, 0, false);
}

IMPLEMENT_SCRIPT(ParticleManager)
