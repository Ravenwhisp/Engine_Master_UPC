#include "pch.h"
#include "DeathParticles.h"


IMPLEMENT_SCRIPT_FIELDS(DeathParticles,
    SERIALIZED_COMPONENT_REF(m_dashTrail, "Dash", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_scytheTrail, "Scythe", ComponentType::TRANSFORM),
    SERIALIZED_ASSET_REF(m_tauntParticle, "Taunt Particle Prefab", AssetType::PREFAB)
)

DeathParticles::DeathParticles(GameObject* owner) : Script(owner)
{

}

void DeathParticles::Update()
{
    if (m_activeTauntParticle == nullptr)
    {
        return;
    }

    m_tauntParticleLifetime -= Time::getDeltaTime();

    if (m_tauntParticleLifetime <= 0.0f)
    {
        GameObjectAPI::removeGameObject(m_activeTauntParticle);
        m_activeTauntParticle = nullptr;
    }
}

Transform* DeathParticles::getTransform(ComponentRef<Transform> controller)
{
    Transform* particleTransform = controller.getReferencedComponent();

    if (particleTransform == nullptr)
    {
        Debug::warn("Missing reference on Death Particles on %s.", GameObjectAPI::getName(getOwner()));
        return nullptr;
    }

    return particleTransform;
}


void DeathParticles::SetDashActive()
{
    if (m_dashTrailController == nullptr) 
    {
        m_dashTrailController = getTransform(m_dashTrail);

        if (m_dashTrailController == nullptr)
        {
            Debug::warn("Dash trail controller not found on Death Particles.");
            return;
        }
    }
    
    TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(m_dashTrailController));
    TrailAPI::generateTrail(trailComponent, true);
}

void DeathParticles::SetScytheActive()
{
    if (m_scytheTrailController == nullptr)
    {
        m_scytheTrailController = getTransform(m_scytheTrail);

        if (m_scytheTrailController == nullptr)
        {
            Debug::warn("Scythe trail controller not found on Death Particles.");
            return;
        }

    }

    TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(m_scytheTrailController));
    TrailAPI::generateTrail(trailComponent, true);
}

void DeathParticles::SetTauntActive(const Vector3& direction)
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        Debug::warn("[DeathParticles] Owner transform not found.");
        return;
    }

    Vector3 spawnPosition = TransformAPI::getGlobalPosition(ownerTransform);

    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() <= 0.0001f)
    {
        Debug::warn("[DeathParticles] Invalid taunt direction.");
        return;
    }

    flatDirection.Normalize();

    const float yawRad = std::atan2(flatDirection.x, flatDirection.z);
    const float yawDeg = yawRad * (180.0f / 3.14159265f);

    Vector3 particleRootRotation(0.0f, yawDeg, 0.0f);

    if (m_activeTauntParticle != nullptr)
    {
        GameObjectAPI::removeGameObject(m_activeTauntParticle);
        m_activeTauntParticle = nullptr;
    }

    m_activeTauntParticle = GameObjectAPI::instantiatePrefab(m_tauntParticle.m_id, spawnPosition, particleRootRotation);

    m_tauntParticleLifetime = 1.0f;
}

void DeathParticles::SetDashInactive()
{
    if (m_dashTrailController == nullptr)
    {
        m_dashTrailController = getTransform(m_dashTrail);

        if (m_dashTrailController == nullptr)
        {
            Debug::warn("Dash trail controller not found on Death Particles.");
            return;
        }

    }

    TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(m_dashTrailController));
    TrailAPI::generateTrail(trailComponent, false);
}

void DeathParticles::SetScytheInactive()
{
    if (m_scytheTrailController == nullptr)
    {
        m_scytheTrailController = getTransform(m_scytheTrail);

        if (m_scytheTrailController == nullptr)
        {
            Debug::warn("Scythe trail controller not found on Death Particles.");
            return;
        }

    }

    TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(m_scytheTrailController));
    TrailAPI::generateTrail(trailComponent, false);
}

void DeathParticles::SetTauntInactive()
{
    if (m_activeTauntParticle != nullptr)
    {
        GameObjectAPI::removeGameObject(m_activeTauntParticle);
        m_activeTauntParticle = nullptr;
    }

    m_tauntParticleLifetime = 0.0f;
}

IMPLEMENT_SCRIPT(DeathParticles)