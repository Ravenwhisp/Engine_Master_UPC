#include "pch.h"
#include "ArthurAttackExecutor.h"

#include "ArthurDetectionAggro.h"
#include "Damageable.h"
#include "PlayerStunState.h"
#include "PlayerState.h"

#include <cmath>

ArthurAttackExecutor::ArthurAttackExecutor(GameObject* owner)
    : Script(owner)
{
}

void ArthurAttackExecutor::Start()
{
    m_arthurDetectionAggro = GameObjectAPI::findScript<ArthurDetectionAggro>(getOwner());

    if (!m_arthurDetectionAggro)
    {
        Debug::error("[ArthurAttackExecutor] ArthurDetectionAggro script not found");
    }
}

void ArthurAttackExecutor::applyDamageInRadius(const Vector3& center, float radius, float damage, const char* sourceName)
{
    if (!m_arthurDetectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_arthurDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_arthurDetectionAggro->getDeathTransform();

    tryDamageTargetInRadius(lyrielTransform, center, radius, damage, sourceName);
    tryDamageTargetInRadius(deathTransform, center, radius, damage, sourceName);
}

void ArthurAttackExecutor::applyDamageAndStunInRadius(const Vector3& center, float radius, float damage, float stunDuration, const char* sourceName)
{
    if (!m_arthurDetectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_arthurDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_arthurDetectionAggro->getDeathTransform();

    tryDamageAndStunTargetInRadius(lyrielTransform, center, radius, damage, stunDuration, sourceName);
    tryDamageAndStunTargetInRadius(deathTransform, center, radius, damage, stunDuration, sourceName);
}

void ArthurAttackExecutor::applyDamageInCone(const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, const char* sourceName)
{
    if (!m_arthurDetectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_arthurDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_arthurDetectionAggro->getDeathTransform();

    tryDamageTargetInCone(lyrielTransform, center, direction, range, halfAngleDegrees, damage, sourceName);
    tryDamageTargetInCone(deathTransform, center, direction, range, halfAngleDegrees, damage, sourceName);
}

bool ArthurAttackExecutor::tryDamageTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, const char* sourceName)
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

void ArthurAttackExecutor::tryDamageAndStunTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, float stunDuration, const char* sourceName)
{
    const bool damaged = tryDamageTargetInRadius(targetTransform, center, radius, damage, sourceName);

    if (!damaged)
    {
        return;
    }

    applyStunToTarget(targetTransform, stunDuration, sourceName);
}

bool ArthurAttackExecutor::tryDamageTargetInCone(Transform* targetTransform, const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, const char* sourceName)
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

    Vector3 toTarget = targetPosition - center;
    toTarget.y = 0.0f;

    const float distanceSquared = toTarget.LengthSquared();
    const float rangeSquared = range * range;

    if (distanceSquared > rangeSquared)
    {
        return false;
    }

    if (distanceSquared < 0.0001f)
    {
        return applyDamageToTarget(targetTransform, damage, sourceName);
    }

    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() < 0.0001f)
    {
        return false;
    }

    toTarget.Normalize();
    flatDirection.Normalize();

    float dot = flatDirection.Dot(toTarget);

    if (dot > 1.0f)
    {
        dot = 1.0f;
    }
    else if (dot < -1.0f)
    {
        dot = -1.0f;
    }

    constexpr float degreesToRadians = 3.14159265f / 180.0f;
    const float minDot = std::cos(halfAngleDegrees * degreesToRadians);

    if (dot < minDot)
    {
        return false;
    }

    return applyDamageToTarget(targetTransform, damage, sourceName);
}

bool ArthurAttackExecutor::applyDamageToTarget(Transform* targetTransform, float damage, const char* sourceName)
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

    Damageable* damageable = GameObjectAPI::findScript<Damageable>(targetObject);
    if (!damageable)
    {
        return false;
    }

    PlayerState* playerState = GameObjectAPI::findScript<PlayerState>(targetObject);
    if (playerState && playerState->isDowned())
    {
        return false;
    }

    damageable->takeDamage(damage);

    Debug::log("[ArthurAttackExecutor] %s damaged '%s' for %.2f.", sourceName, GameObjectAPI::getName(targetObject), damage);
    return true;
}

void ArthurAttackExecutor::applyStunToTarget(Transform* targetTransform, float stunDuration, const char* sourceName)
{
    if (!targetTransform)
    {
        return;
    }

    if (stunDuration <= 0.0f)
    {
        return;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);
    if (!targetObject)
    {
        return;
    }

    PlayerStunState* stunState = GameObjectAPI::findScript<PlayerStunState>(targetObject);
    if (!stunState)
    {
        return;
    }

    stunState->enterStun(stunDuration);

    Debug::log("[ArthurAttackExecutor] %s stunned '%s' for %.2f seconds.", sourceName, GameObjectAPI::getName(targetObject), stunDuration);
}

IMPLEMENT_SCRIPT(ArthurAttackExecutor)