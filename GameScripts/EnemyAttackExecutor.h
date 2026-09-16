#pragma once

#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

class EnemyDetectionAggro;
class PlayerStunState;

class EnemyAttackExecutor : public Script
{
    DECLARE_SCRIPT(EnemyAttackExecutor)

public:
    explicit EnemyAttackExecutor(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnGameStop() override;

    void setNextPlayerHitVfx(const AssetId& prefabId);
    void playPlayerHitVfx(Transform* targetTransform, const AssetId& prefabId = AssetId());

    bool damageTarget(Transform* targetTransform, float damage, const char* sourceName);

    void applyDamageInRadius(
        const Vector3& center,
        float radius,
        float damage,
        const char* sourceName
    );

    void applyDamageAndStunInRadius(
        const Vector3& center,
        float radius,
        float damage,
        float stunDuration,
        const char* sourceName
    );

    int applyDamageInCone(
        const Vector3& center,
        const Vector3& direction,
        float range,
        float halfAngleDegrees,
        float damage,
        const char* sourceName
    );

    void applyDamageAndStunInCone(
        const Vector3& center,
        const Vector3& direction,
        float range,
        float halfAngleDegrees,
        float damage,
        float stunDuration,
        const char* sourceName
    );

    int applyDamageInRectangle(
        const Vector3& origin,
        const Vector3& direction,
        float length,
        float width,
        float damage,
        const char* sourceName
    );

    bool tryDamageTargetInRadius(
        Transform* targetTransform,
        const Vector3& center,
        float radius,
        float damage,
        const char* sourceName
    );

    void tryDamageAndStunTargetInRadius(
        Transform* targetTransform,
        const Vector3& center,
        float radius,
        float damage,
        float stunDuration,
        const char* sourceName
    );

    bool tryDamageTargetInCone(
        Transform* targetTransform,
        const Vector3& center,
        const Vector3& direction,
        float range,
        float halfAngleDegrees,
        float damage,
        const char* sourceName
    );

    bool tryDamageTargetInRectangle(
        Transform* targetTransform,
        const Vector3& origin,
        const Vector3& direction,
        float length,
        float width,
        float damage,
        const char* sourceName
    );

    void tryDamageAndStunSingleTargetInCone(
        Transform* targetTransform,
        const Vector3& center,
        const Vector3& direction,
        float range,
        float halfAngleDegrees,
        float damage,
        float stunDuration,
        const char* sourceName
    );

private:
    bool applyDamageToTarget(
        Transform* targetTransform,
        float damage,
        const char* sourceName
    );

    void applyStunToTarget(
        Transform* targetTransform,
        float stunDuration,
        const char* sourceName
    );

private:
    EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;

    AssetId m_nextPlayerHitVfxOverride;
    ParticleLifecycle::TimedParticleTracker m_timedHitVfx;
};