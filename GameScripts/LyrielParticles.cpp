#include "pch.h"
#include "LyrielParticles.h"
#include "ParticleLifecycle.h"

IMPLEMENT_SCRIPT_FIELDS(LyrielParticles,
    SERIALIZED_COMPONENT_REF(m_dashTrail, "Dash", ComponentType::TRANSFORM),
    SERIALIZED_STRING(m_chargeGlowPath, "Charge Glow Prefab Path"),
    SERIALIZED_ASSET_REF(m_chargeGlowPrefab, "Charge Glow Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_dashParticlePath, "Dash Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_dashParticlePrefab, "Dash Particle Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_hitFlashPath, "Hit Flash Prefab Path"),
    SERIALIZED_ASSET_REF(m_hitFlashPrefab, "Hit Flash Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_bowAnchorName, "Bow Anchor Name")
)

LyrielParticles::LyrielParticles(GameObject* owner)
    : Script(owner)
{
}

void LyrielParticles::Start()
{
}

void LyrielParticles::OnGameStop()
{
    ParticleLifecycle::destroy(m_chargeGlowInstance);
    ParticleLifecycle::destroy(m_dashParticleInstance);
    m_timedOneShots.clear();
    m_chargeGlowActive = false;
    m_dashParticleActive = false;
}

void LyrielParticles::Update()
{
    m_timedOneShots.update(Time::getDeltaTime());
    syncActiveParticles();
}

Transform* LyrielParticles::getTransform(ComponentRef<Transform> controller)
{
    return controller.getReferencedComponent();
}

Transform* LyrielParticles::findBowTransform() const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        return nullptr;
    }

    if (Transform* bow = ParticleLifecycle::findChildRecursive(ownerTransform, m_bowAnchorName.c_str()))
    {
        return bow;
    }

    if (Transform* arbalet = ParticleLifecycle::findChildRecursive(ownerTransform, "arbalet"))
    {
        return arbalet;
    }

    return ownerTransform;
}

void LyrielParticles::syncActiveParticles()
{
    if (m_chargeGlowActive)
    {
        ParticleLifecycle::syncToTransform(m_chargeGlowInstance, findBowTransform());
    }

    if (m_dashParticleActive)
    {
        ParticleLifecycle::syncToTransform(m_dashParticleInstance, GameObjectAPI::getTransform(getOwner()));
    }
}

void LyrielParticles::SetDashActive()
{
    if (m_dashTrailController == nullptr)
    {
        m_dashTrailController = getTransform(m_dashTrail);
    }

    if (m_dashTrailController != nullptr)
    {
        const int childCount = TransformAPI::getChildCount(m_dashTrailController);
        for (int i = 0; i < childCount; ++i)
        {
            Transform* child = TransformAPI::getChild(m_dashTrailController, i);
            TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(child));
            TrailAPI::generateTrail(trailComponent, true);
        }
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    const Vector3 position = ownerTransform != nullptr ? TransformAPI::getGlobalPosition(ownerTransform) : Vector3::Zero;
    const Vector3 rotation = ownerTransform != nullptr ? TransformAPI::getGlobalEulerDegrees(ownerTransform) : Vector3::Zero;

    ParticleLifecycle::ensurePersistent(m_dashParticleInstance, m_dashParticlePrefab.m_id, position, rotation, nullptr);
    ParticleLifecycle::syncToTransform(m_dashParticleInstance, ownerTransform);
    ParticleLifecycle::activate(m_dashParticleInstance);
    m_dashParticleActive = m_dashParticleInstance != nullptr;
}

void LyrielParticles::SetDashInactive()
{
    if (m_dashTrailController == nullptr)
    {
        m_dashTrailController = getTransform(m_dashTrail);
    }

    if (m_dashTrailController != nullptr)
    {
        const int childCount = TransformAPI::getChildCount(m_dashTrailController);
        for (int i = 0; i < childCount; ++i)
        {
            Transform* child = TransformAPI::getChild(m_dashTrailController, i);
            TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(child));
            TrailAPI::generateTrail(trailComponent, false);
        }
    }

    ParticleLifecycle::deactivate(m_dashParticleInstance);
    m_dashParticleActive = false;
}

void LyrielParticles::SetChargeActive()
{
    Transform* bowTransform = findBowTransform();
    const Vector3 position = bowTransform != nullptr ? TransformAPI::getGlobalPosition(bowTransform) : Vector3::Zero;
    const Vector3 rotation = bowTransform != nullptr ? TransformAPI::getGlobalEulerDegrees(bowTransform) : Vector3::Zero;

    ParticleLifecycle::ensurePersistent(m_chargeGlowInstance, m_chargeGlowPrefab.m_id, position, rotation, nullptr);
    ParticleLifecycle::syncToTransform(m_chargeGlowInstance, bowTransform);
    ParticleLifecycle::activate(m_chargeGlowInstance);
    m_chargeGlowActive = m_chargeGlowInstance != nullptr;
}

void LyrielParticles::SetChargeInactive()
{
    ParticleLifecycle::deactivate(m_chargeGlowInstance);
    m_chargeGlowActive = false;
}

void LyrielParticles::playHitFlash(const Vector3& position)
{
    ParticleLifecycle::spawnOneShotTimed(
        m_timedOneShots,
        m_hitFlashPrefab.m_id,
        position
    );
}

IMPLEMENT_SCRIPT(LyrielParticles)
