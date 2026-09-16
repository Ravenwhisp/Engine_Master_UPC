#include "pch.h"
#include "EnemyAttackExecutor.h"

#include "EnemyDetectionAggro.h"
#include "Damageable.h"
#include "PlayerStunState.h"
#include "PlayerState.h"

#include "AssetType.h"

#include <cmath>
#include <cstring>

namespace
{
    // Assets/Prefabs/Particles/VFXRemake/MainCharDamage/PS_MainCharDamage.prefab
    AssetId getMainCharDamagePrefabId()
    {
        return AssetId(
            15943977396033392323ULL,
            "1bf8ced93d9fd200d3de5178dbb73cad",
            AssetType::PREFAB
        );
    }
}

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

void EnemyAttackExecutor::Update()
{
    m_timedHitVfx.update(Time::getDeltaTime());
}

void EnemyAttackExecutor::OnGameStop()
{
    m_timedHitVfx.clear();
    m_nextPlayerHitVfxOverride = AssetId();
}

void EnemyAttackExecutor::setNextPlayerHitVfx(const AssetId& prefabId)
{
    m_nextPlayerHitVfxOverride = prefabId;
}

void EnemyAttackExecutor::playPlayerHitVfx(Transform* targetTransform, const AssetId& prefabId)
{
    AssetId resolvedPrefab = prefabId.isValid() ? prefabId : getMainCharDamagePrefabId();

    if (!resolvedPrefab.isValid() || targetTransform == nullptr)
    {
        return;
    }

    const Vector3 position = TransformAPI::getGlobalPosition(targetTransform);

    ParticleLifecycle::spawnOneShotTimed(
        m_timedHitVfx,
        resolvedPrefab,
        position,
        Vector3::Zero,
        ParticleLifecycle::kDefaultOneShotLifetime
    );
}

bool EnemyAttackExecutor::damageTarget(Transform* targetTransform, float damage, const char* sourceName)
{
    return applyDamageToTarget(targetTransform, damage, sourceName);
}

void EnemyAttackExecutor::applyDamageInRadius(
    const Vector3& center,
    float radius,
    float damage,
    const char* sourceName
)
{
    if (!m_enemyDetectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    tryDamageTargetInRadius(
        lyrielTransform,
        center,
        radius,
        damage,
        sourceName
    );

    tryDamageTargetInRadius(
        deathTransform,
        center,
        radius,
        damage,
        sourceName
    );
}

void EnemyAttackExecutor::applyDamageAndStunInRadius(
    const Vector3& center,
    float radius,
    float damage,
    float stunDuration,
    const char* sourceName
)
{
    if (!m_enemyDetectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    tryDamageAndStunTargetInRadius(
        lyrielTransform,
        center,
        radius,
        damage,
        stunDuration,
        sourceName
    );

    tryDamageAndStunTargetInRadius(
        deathTransform,
        center,
        radius,
        damage,
        stunDuration,
        sourceName
    );
}

int EnemyAttackExecutor::applyDamageInCone(
    const Vector3& center,
    const Vector3& direction,
    float range,
    float halfAngleDegrees,
    float damage,
    const char* sourceName
)
{
    if (!m_enemyDetectionAggro)
    {
        return 0;
    }

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    int hits = 0;

    if (tryDamageTargetInCone(
        lyrielTransform,
        center,
        direction,
        range,
        halfAngleDegrees,
        damage,
        sourceName
    ))
    {
        ++hits;
    }

    if (tryDamageTargetInCone(
        deathTransform,
        center,
        direction,
        range,
        halfAngleDegrees,
        damage,
        sourceName
    ))
    {
        ++hits;
    }

    return hits;
}

void EnemyAttackExecutor::applyDamageAndStunInCone(
    const Vector3& center,
    const Vector3& direction,
    float range,
    float halfAngleDegrees,
    float damage,
    float stunDuration,
    const char* sourceName
)
{
    if (!m_enemyDetectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    const bool lyrielDamaged = tryDamageTargetInCone(
        lyrielTransform,
        center,
        direction,
        range,
        halfAngleDegrees,
        damage,
        sourceName
    );

    if (lyrielDamaged)
    {
        applyStunToTarget(
            lyrielTransform,
            stunDuration,
            sourceName
        );
    }

    const bool deathDamaged = tryDamageTargetInCone(
        deathTransform,
        center,
        direction,
        range,
        halfAngleDegrees,
        damage,
        sourceName
    );

    if (deathDamaged)
    {
        applyStunToTarget(
            deathTransform,
            stunDuration,
            sourceName
        );
    }
}

int EnemyAttackExecutor::applyDamageInRectangle(
    const Vector3& origin,
    const Vector3& direction,
    float length,
    float width,
    float damage,
    const char* sourceName
)
{
    if (!m_enemyDetectionAggro)
    {
        return 0;
    }

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    int hits = 0;

    if (tryDamageTargetInRectangle(
        lyrielTransform,
        origin,
        direction,
        length,
        width,
        damage,
        sourceName
    ))
    {
        ++hits;
    }

    if (tryDamageTargetInRectangle(
        deathTransform,
        origin,
        direction,
        length,
        width,
        damage,
        sourceName
    ))
    {
        ++hits;
    }

    return hits;
}

bool EnemyAttackExecutor::tryDamageTargetInRadius(
    Transform* targetTransform,
    const Vector3& center,
    float radius,
    float damage,
    const char* sourceName
)
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

    return applyDamageToTarget(
        targetTransform,
        damage,
        sourceName
    );
}

void EnemyAttackExecutor::tryDamageAndStunTargetInRadius(
    Transform* targetTransform,
    const Vector3& center,
    float radius,
    float damage,
    float stunDuration,
    const char* sourceName
)
{
    const bool damaged = tryDamageTargetInRadius(
        targetTransform,
        center,
        radius,
        damage,
        sourceName
    );

    if (!damaged)
    {
        return;
    }

    applyStunToTarget(
        targetTransform,
        stunDuration,
        sourceName
    );
}

bool EnemyAttackExecutor::tryDamageTargetInCone(
    Transform* targetTransform,
    const Vector3& center,
    const Vector3& direction,
    float range,
    float halfAngleDegrees,
    float damage,
    const char* sourceName
)
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
        return applyDamageToTarget(
            targetTransform,
            damage,
            sourceName
        );
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

    return applyDamageToTarget(
        targetTransform,
        damage,
        sourceName
    );
}

bool EnemyAttackExecutor::tryDamageTargetInRectangle(
    Transform* targetTransform,
    const Vector3& origin,
    const Vector3& direction,
    float length,
    float width,
    float damage,
    const char* sourceName
)
{
    if (!targetTransform)
    {
        return false;
    }

    if (length <= 0.0f || width <= 0.0f)
    {
        return false;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);

    if (!targetObject)
    {
        return false;
    }

    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() < 0.0001f)
    {
        return false;
    }

    flatDirection.Normalize();

    Vector3 rightDirection(
        flatDirection.z,
        0.0f,
        -flatDirection.x
    );

    Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    Vector3 toTarget = targetPosition - origin;
    toTarget.y = 0.0f;

    const float forwardDistance = flatDirection.Dot(toTarget);

    if (forwardDistance < 0.0f ||
        forwardDistance > length)
    {
        return false;
    }

    const float lateralDistance = rightDirection.Dot(toTarget);
    const float halfWidth = width * 0.5f;

    if (lateralDistance < -halfWidth ||
        lateralDistance > halfWidth)
    {
        return false;
    }

    return applyDamageToTarget(
        targetTransform,
        damage,
        sourceName
    );
}

void EnemyAttackExecutor::tryDamageAndStunSingleTargetInCone(
    Transform* targetTransform,
    const Vector3& center,
    const Vector3& direction,
    float range,
    float halfAngleDegrees,
    float damage,
    float stunDuration,
    const char* sourceName
)
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

    applyStunToTarget(
        targetTransform,
        stunDuration,
        sourceName
    );
}

bool EnemyAttackExecutor::applyDamageToTarget(
    Transform* targetTransform,
    float damage,
    const char* sourceName
)
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

    Debug::log(
        "[EnemyAttackExecutor] %s damaged '%s' for %.2f.",
        sourceName,
        GameObjectAPI::getName(targetObject),
        damage
    );

    AssetId hitVfx = m_nextPlayerHitVfxOverride.isValid()
        ? m_nextPlayerHitVfxOverride
        : getMainCharDamagePrefabId();

    m_nextPlayerHitVfxOverride = AssetId();

    const bool skipDefaultHeavySwipeVfx = sourceName != nullptr && strcmp(sourceName, "HeavySwipe") == 0;

    if (!skipDefaultHeavySwipeVfx)
    {
        playPlayerHitVfx(targetTransform, hitVfx);
    }

    return true;
}

void EnemyAttackExecutor::applyStunToTarget(
    Transform* targetTransform,
    float stunDuration,
    const char* sourceName
)
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

    Debug::log(
        "[EnemyAttackExecutor] %s stunned '%s' for %.2f seconds.",
        sourceName,
        GameObjectAPI::getName(targetObject),
        stunDuration
    );
}

IMPLEMENT_SCRIPT(EnemyAttackExecutor)