#include "pch.h"
#include "DeathParticles.h"
#include "ParticleLifecycle.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(DeathParticles,
    SERIALIZED_COMPONENT_REF(m_dashTrail, "Dash", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_scytheTrail, "Scythe", ComponentType::TRANSFORM),
    SERIALIZED_STRING(m_tauntParticlePath, "Taunt Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_tauntParticle, "Taunt Particle Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_dashParticlePath, "Dash Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_dashParticlePrefab, "Dash Particle Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_chargeGlowPath, "Charge Glow Prefab Path"),
    SERIALIZED_ASSET_REF(m_chargeGlowPrefab, "Charge Glow Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_hitFlashPath, "Hit Flash Prefab Path"),
    SERIALIZED_ASSET_REF(m_hitFlashPrefab, "Hit Flash Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_chargedHitFlashPath, "Charged Hit Flash Prefab Path"),
    SERIALIZED_ASSET_REF(m_chargedHitFlashPrefab, "Charged Hit Flash Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_scytheAnchorName, "Scythe Anchor Name")
)

DeathParticles::DeathParticles(GameObject* owner)
    : Script(owner)
{
}

void DeathParticles::Start()
{
}

void DeathParticles::OnGameStop()
{
    ParticleLifecycle::destroy(m_activeTauntParticle);
    ParticleLifecycle::destroy(m_dashParticleInstance);
    ParticleLifecycle::destroy(m_chargeGlowInstance);
    m_timedOneShots.clear();
    m_tauntParticleActive = false;
    m_dashParticleActive = false;
    m_chargeGlowActive = false;
    m_tauntParticleLifetime = 0.0f;
}

void DeathParticles::Update()
{
    m_timedOneShots.update(Time::getDeltaTime());
    syncActiveParticles();

    if (!m_tauntParticleActive)
    {
        return;
    }

    m_tauntParticleLifetime -= Time::getDeltaTime();

    if (m_tauntParticleLifetime <= 0.0f)
    {
        SetTauntInactive();
    }
}

Transform* DeathParticles::getTransform(ComponentRef<Transform> controller)
{
    return controller.getReferencedComponent();
}

Transform* DeathParticles::findScytheTransform() const
{
    if (Transform* scytheTrail = m_scytheTrail.getReferencedComponent())
    {
        return scytheTrail;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        return nullptr;
    }

    if (Transform* named = ParticleLifecycle::findChildRecursive(ownerTransform, m_scytheAnchorName.c_str()))
    {
        return named;
    }

    if (Transform* trail = ParticleLifecycle::findChildRecursive(ownerTransform, "scythe_trail_controller"))
    {
        return trail;
    }

    return ownerTransform;
}

void DeathParticles::syncActiveParticles()
{
    if (m_chargeGlowActive)
    {
        ParticleLifecycle::syncToTransform(m_chargeGlowInstance, findScytheTransform());
    }

    if (m_dashParticleActive)
    {
        ParticleLifecycle::syncToTransform(m_dashParticleInstance, GameObjectAPI::getTransform(getOwner()));
    }
}

void DeathParticles::ensureTauntParticle(const Vector3& position, const Vector3& rotation)
{
    ParticleLifecycle::ensurePersistent(m_activeTauntParticle, m_tauntParticle.m_id, position, rotation, nullptr);
}

void DeathParticles::SetDashActive()
{
    if (m_dashTrailController == nullptr)
    {
        m_dashTrailController = getTransform(m_dashTrail);
    }

    if (m_dashTrailController != nullptr)
    {
        TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(m_dashTrailController));
        TrailAPI::generateTrail(trailComponent, true);
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    const Vector3 position = ownerTransform != nullptr ? TransformAPI::getGlobalPosition(ownerTransform) : Vector3::Zero;
    const Vector3 rotation = ownerTransform != nullptr ? TransformAPI::getGlobalEulerDegrees(ownerTransform) : Vector3::Zero;

    ParticleLifecycle::ensurePersistent(m_dashParticleInstance, m_dashParticlePrefab.m_id, position, rotation, nullptr);
    ParticleLifecycle::syncToTransform(m_dashParticleInstance, ownerTransform);
    ParticleLifecycle::activate(m_dashParticleInstance);
    m_dashParticleActive = m_dashParticleInstance != nullptr;
}

void DeathParticles::SetDashInactive()
{
    if (m_dashTrailController == nullptr)
    {
        m_dashTrailController = getTransform(m_dashTrail);
    }

    if (m_dashTrailController != nullptr)
    {
        TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(m_dashTrailController));
        TrailAPI::generateTrail(trailComponent, false);
    }

    ParticleLifecycle::deactivate(m_dashParticleInstance);
    m_dashParticleActive = false;
}

void DeathParticles::SetScytheActive()
{
    if (m_scytheTrailController == nullptr)
    {
        m_scytheTrailController = getTransform(m_scytheTrail);
    }

    if (m_scytheTrailController == nullptr)
    {
        return;
    }

    TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(m_scytheTrailController));
    TrailAPI::generateTrail(trailComponent, true);
}

void DeathParticles::SetScytheInactive()
{
    if (m_scytheTrailController == nullptr)
    {
        m_scytheTrailController = getTransform(m_scytheTrail);
    }

    if (m_scytheTrailController == nullptr)
    {
        return;
    }

    TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(m_scytheTrailController));
    TrailAPI::generateTrail(trailComponent, false);
}

void DeathParticles::SetChargeActive()
{
    Transform* scytheTransform = findScytheTransform();
    const Vector3 position = scytheTransform != nullptr ? TransformAPI::getGlobalPosition(scytheTransform) : Vector3::Zero;
    const Vector3 rotation = scytheTransform != nullptr ? TransformAPI::getGlobalEulerDegrees(scytheTransform) : Vector3::Zero;

    ParticleLifecycle::ensurePersistent(m_chargeGlowInstance, m_chargeGlowPrefab.m_id, position, rotation, nullptr);
    ParticleLifecycle::syncToTransform(m_chargeGlowInstance, scytheTransform);
    ParticleLifecycle::activate(m_chargeGlowInstance);
    m_chargeGlowActive = m_chargeGlowInstance != nullptr;
}

void DeathParticles::SetChargeInactive()
{
    ParticleLifecycle::deactivate(m_chargeGlowInstance);
    m_chargeGlowActive = false;
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

    ensureTauntParticle(spawnPosition, particleRootRotation);

    if (m_activeTauntParticle == nullptr)
    {
        Debug::warn("[DeathParticles] Could not instantiate taunt particle.");
        return;
    }

    Transform* particleTransform = GameObjectAPI::getTransform(m_activeTauntParticle);
    if (particleTransform != nullptr)
    {
        TransformAPI::setGlobalPosition(particleTransform, spawnPosition);
        TransformAPI::setGlobalRotationEuler(particleTransform, particleRootRotation);
    }

    ParticleLifecycle::activate(m_activeTauntParticle);
    m_tauntParticleActive = true;
    m_tauntParticleLifetime = 1.0f;
}

void DeathParticles::SetTauntInactive()
{
    ParticleLifecycle::deactivate(m_activeTauntParticle);
    m_tauntParticleActive = false;
    m_tauntParticleLifetime = 0.0f;
}

void DeathParticles::playHitFlash(const Vector3& position)
{
    ParticleLifecycle::spawnOneShotTimed(
        m_timedOneShots,
        m_hitFlashPrefab.m_id,
        position
    );
}

void DeathParticles::playChargedHitFlash(const Vector3& position)
{
    ParticleLifecycle::spawnOneShotTimed(
        m_timedOneShots,
        m_chargedHitFlashPrefab.m_id,
        position
    );
}

IMPLEMENT_SCRIPT(DeathParticles)
