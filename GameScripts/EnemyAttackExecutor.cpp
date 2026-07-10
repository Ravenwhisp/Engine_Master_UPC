#include "pch.h"
#include "EnemyAttackExecutor.h"

#include "EnemyDetectionAggro.h"
#include "Damageable.h"
#include "PlayerStunState.h"
#include "PlayerState.h"

#include <cmath>

EnemyAttackExecutor::EnemyAttackExecutor(GameObject* owner)
    : Script(owner)
{
}

void EnemyAttackExecutor::Start()
{
    m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());

    if (!m_enemyDetectionAggro)
    {
        Debug::error("[EnemyAttackExecutor] EnemyDetectionAggro script not found");
    }
}

void EnemyAttackExecutor::applyDamageInRadius(const Vector3& center, float radius, float damage, const char* sourceName)
{
    if (!m_enemyDetectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    tryDamageTargetInRadius(lyrielTransform, center, radius, damage, sourceName);
    tryDamageTargetInRadius(deathTransform, center, radius, damage, sourceName);
}

void EnemyAttackExecutor::applyDamageAndStunInRadius(const Vector3& center, float radius, float damage, float stunDuration, const char* sourceName)
{
    if (!m_enemyDetectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    tryDamageAndStunTargetInRadius(lyrielTransform, center, radius, damage, stunDuration, sourceName);
    tryDamageAndStunTargetInRadius(deathTransform, center, radius, damage, stunDuration, sourceName);
}

int EnemyAttackExecutor::applyDamageInCone(const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, const char* sourceName)
{
    if (!m_enemyDetectionAggro)
    {
        return 0;
    }

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    int hits = 0;
    if (tryDamageTargetInCone(lyrielTransform, center, direction, range, halfAngleDegrees, damage, sourceName)) ++hits;
    if (tryDamageTargetInCone(deathTransform, center, direction, range, halfAngleDegrees, damage, sourceName)) ++hits;
    return hits;
}

bool EnemyAttackExecutor::tryDamageTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, const char* sourceName)
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

void EnemyAttackExecutor::tryDamageAndStunTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, float stunDuration, const char* sourceName)
{
    const bool damaged = tryDamageTargetInRadius(targetTransform, center, radius, damage, sourceName);

    if (!damaged)
    {
        return;
    }

    applyStunToTarget(targetTransform, stunDuration, sourceName);
}

bool EnemyAttackExecutor::tryDamageTargetInCone(Transform* targetTransform, const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, const char* sourceName)
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

void EnemyAttackExecutor::tryDamageAndStunSingleTargetInCone(Transform* targetTransform, const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, float stunDuration, const char* sourceName)
{
    const bool damaged = tryDamageTargetInCone(
        targetTransform,
        center,
        direction,
        range,
        halfAngleDegrees,
        damage,
        sourceName
    );

    if (!damaged)
    {
        return;
    }

    applyStunToTarget(targetTransform, stunDuration, sourceName);
}

bool EnemyAttackExecutor::applyDamageToTarget(Transform* targetTransform, float damage, const char* sourceName)
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

    Debug::log("[EnemyAttackExecutor] %s damaged '%s' for %.2f.", sourceName, GameObjectAPI::getName(targetObject), damage);
    return true;
}

void EnemyAttackExecutor::applyStunToTarget(Transform* targetTransform, float stunDuration, const char* sourceName)
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

    Debug::log("[EnemyAttackExecutor] %s stunned '%s' for %.2f seconds.", sourceName, GameObjectAPI::getName(targetObject), stunDuration);
}

IMPLEMENT_SCRIPT(EnemyAttackExecutor)