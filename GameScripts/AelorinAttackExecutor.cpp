#include "pch.h"
#include "AelorinAttackExecutor.h"

#include "AelorinDetectionAggro.h"
#include "Damageable.h"
#include "PlayerState.h"

#include <cmath>

AelorinAttackExecutor::AelorinAttackExecutor(GameObject* owner)
    : Script(owner)
{
}

void AelorinAttackExecutor::Start()
{
    m_detectionAggro = GameObjectAPI::findScript<AelorinDetectionAggro>(getOwner());

    if (!m_detectionAggro)
    {
        Debug::error("[AelorinAttackExecutor] AelorinDetectionAggro script not found");
    }
}

bool AelorinAttackExecutor::isValidPlayerTarget(Transform* targetTransform) const
{
    if (!targetTransform)
    {
        return false;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);
    if (!targetObject)
    {
        return false;
    }

    PlayerState* playerState = GameObjectAPI::findScript<PlayerState>(targetObject);
    if (playerState && playerState->isDowned())
    {
        return false;
    }

    return true;
}

bool AelorinAttackExecutor::isValidDamageTarget(Transform* targetTransform) const
{
    if (!isValidPlayerTarget(targetTransform))
    {
        return false;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);
    if (!targetObject)
    {
        return false;
    }

    Damageable* damageable = GameObjectAPI::findScript<Damageable>(targetObject);
    if (!damageable)
    {
        return false;
    }

    return true;
}

void AelorinAttackExecutor::applyDamageInRadius(const Vector3& center, float radius, float damage, const char* sourceName)
{
    if (!m_detectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_detectionAggro->getLyrielTransform();
    Transform* deathTransform = m_detectionAggro->getDeathTransform();

    tryDamageTargetInRadius(lyrielTransform, center, radius, damage, sourceName);
    tryDamageTargetInRadius(deathTransform, center, radius, damage, sourceName);
}

bool AelorinAttackExecutor::tryDamageTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, const char* sourceName)
{
    if (!targetTransform)
    {
        return false;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);
    if (!targetObject)
    {
        return false;
    }

    Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    Vector3 difference = targetPosition - center;
    difference.y = 0.0f;

    const float distanceSquared = difference.LengthSquared();
    const float radiusSquared = radius * radius;

    if (distanceSquared > radiusSquared)
    {
        return false;
    }

    return applyDamageToTarget(targetTransform, damage, sourceName);
}

void AelorinAttackExecutor::applyDamageInBeam(const Vector3& origin, const Vector3& direction, float length, float width, float damage, const char* sourceName)
{
    if (!m_detectionAggro)
    {
        return;
    }

    Vector3 beamDirection = direction;
    beamDirection.y = 0.0f;

    if (beamDirection.LengthSquared() <= 0.00001f)
    {
        return;
    }

    beamDirection.Normalize();

    const float halfWidth = width * 0.5f;

    tryDamageTargetInBeam(m_detectionAggro->getLyrielTransform(), origin, beamDirection, length, halfWidth, damage, sourceName);
    tryDamageTargetInBeam(m_detectionAggro->getDeathTransform(), origin, beamDirection, length, halfWidth, damage, sourceName);
}

bool AelorinAttackExecutor::tryDamageTargetInBeam(Transform* targetTransform, const Vector3& origin, const Vector3& direction, float length, float halfWidth, float damage, const char* sourceName)
{
    if (!targetTransform)
    {
        return false;
    }

    const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    Vector3 toTarget = targetPosition - origin;
    toTarget.y = 0.0f;

    // Beam distance
    const float forwardDistance = toTarget.Dot(direction);

    // Behind the boss or beyond the end of the beam
    if (forwardDistance < 0.0f || forwardDistance > length)
    {
        return false;
    }

    // Closest point on the beam center line
    const Vector3 closestPoint = direction * forwardDistance;
    const Vector3 lateralOffset = toTarget - closestPoint;

    const float lateralDistanceSquared = lateralOffset.LengthSquared();

    if (lateralDistanceSquared > halfWidth * halfWidth)
    {
        return false;
    }

    return applyDamageToTarget(targetTransform, damage, sourceName);
}

bool AelorinAttackExecutor::applyDamageToTarget(Transform* targetTransform, float damage, const char* sourceName)
{
    if (!isValidDamageTarget(targetTransform))
    {
        return false;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);
    Damageable* damageable = GameObjectAPI::findScript<Damageable>(targetObject);

    damageable->takeDamage(damage);

    Debug::log("[AelorinAttackExecutor] %s damaged '%s' for %.2f.", sourceName, GameObjectAPI::getName(targetObject), damage);
    return true;
}

IMPLEMENT_SCRIPT(AelorinAttackExecutor)