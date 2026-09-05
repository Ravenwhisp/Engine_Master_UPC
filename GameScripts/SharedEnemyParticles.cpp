#include "pch.h"
#include "SharedEnemyParticles.h"
#include "ParticleLifecycle.h"

IMPLEMENT_SCRIPT_FIELDS(SharedEnemyParticles,
    SERIALIZED_STRING(m_movementParticlePath, "Movement Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_movementParticlePrefab, "Movement Particle Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(m_movementParticleYOffset, "Movement Particle Y Offset", -5.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_movementParticleForwardOffset, "Movement Particle Forward Offset", -5.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_movementNotificationTimeout, "Movement Notification Timeout", 0.01f, 1.0f, 0.01f)
)

SharedEnemyParticles::SharedEnemyParticles(GameObject* owner)
    : Script(owner)
{
}

void SharedEnemyParticles::Start()
{
    m_ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (m_ownerTransform == nullptr)
    {
        Debug::warn("SharedEnemyParticles on '%s' could not find the owner transform.", GameObjectAPI::getName(getOwner()));
    }
}

void SharedEnemyParticles::OnGameStop()
{
    ParticleLifecycle::destroy(m_movementParticle);
    m_movementParticleTransform = nullptr;
    m_movementParticleActive = false;
}

void SharedEnemyParticles::Update()
{
    if (m_movementNotificationTimer > 0.0f)
    {
        m_movementNotificationTimer -= Time::getDeltaTime();
    }

    if (!m_movementParticleActive)
    {
        return;
    }

    if (m_movementNotificationTimer <= 0.0f)
    {
        stopMovementParticle();
        return;
    }

    updateMovementParticle();
}

void SharedEnemyParticles::notifyMoving()
{
    m_movementNotificationTimer = m_movementNotificationTimeout;

    if (!m_movementParticleActive)
    {
        startMovementParticle();
    }
}

void SharedEnemyParticles::ensureMovementParticle()
{
    if (m_ownerTransform == nullptr)
    {
        m_ownerTransform = GameObjectAPI::getTransform(getOwner());
    }

    ParticleLifecycle::ensurePersistent(
        m_movementParticle,
        m_movementParticlePrefab.m_id,
        getMovementParticlePosition(),
        getOwnerRotation(),
        nullptr
    );

    if (m_movementParticle != nullptr)
    {
        m_movementParticleTransform = GameObjectAPI::getTransform(m_movementParticle);
    }
}

void SharedEnemyParticles::startMovementParticle()
{
    ensureMovementParticle();

    if (m_movementParticle == nullptr)
    {
        Debug::warn("SharedEnemyParticles on '%s' could not instantiate the movement particle.", GameObjectAPI::getName(getOwner()));
        return;
    }

    updateMovementParticle();
    ParticleLifecycle::activate(m_movementParticle);
    m_movementParticleActive = true;
}

void SharedEnemyParticles::stopMovementParticle()
{
    ParticleLifecycle::deactivate(m_movementParticle);
    m_movementNotificationTimer = 0.0f;
    m_movementParticleActive = false;
}

void SharedEnemyParticles::updateMovementParticle()
{
    if (m_movementParticleTransform == nullptr)
    {
        return;
    }

    TransformAPI::setGlobalPosition(m_movementParticleTransform, getMovementParticlePosition());
    TransformAPI::setGlobalRotationEuler(m_movementParticleTransform, getOwnerRotation());
}

Vector3 SharedEnemyParticles::getMovementParticlePosition() const
{
    if (m_ownerTransform == nullptr)
    {
        return Vector3::Zero;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(m_ownerTransform);
    const Vector3 ownerForward = TransformAPI::getForward(m_ownerTransform);

    return Vector3(ownerPosition.x + ownerForward.x * m_movementParticleForwardOffset, ownerPosition.y + m_movementParticleYOffset, ownerPosition.z + ownerForward.z * m_movementParticleForwardOffset);
}

Vector3 SharedEnemyParticles::getOwnerRotation() const
{
    if (m_ownerTransform == nullptr)
    {
        return Vector3::Zero;
    }

    return TransformAPI::getGlobalEulerDegrees(m_ownerTransform);
}

IMPLEMENT_SCRIPT(SharedEnemyParticles)
