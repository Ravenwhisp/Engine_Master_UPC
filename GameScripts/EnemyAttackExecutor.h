#pragma once

#include "ScriptAPI.h"

class EnemyDetectionAggro;
class PlayerStunState;

class EnemyAttackExecutor : public Script
{
    DECLARE_SCRIPT(EnemyAttackExecutor)

public:
    explicit EnemyAttackExecutor(GameObject* owner);

    void Start() override;

    void applyDamageInRadius(const Vector3& center, float radius, float damage, const char* sourceName);
    void applyDamageAndStunInRadius(const Vector3& center, float radius, float damage, float stunDuration, const char* sourceName);
    int applyDamageInCone(const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, const char* sourceName);   // returns how many targets were hit

    bool tryDamageTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, const char* sourceName);
    void tryDamageAndStunTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, float stunDuration, const char* sourceName);
    bool tryDamageTargetInCone(Transform* targetTransform, const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, const char* sourceName);

    void tryDamageAndStunSingleTargetInCone(Transform* targetTransform, const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, float stunDuration, const char* sourceName);

private:
    bool applyDamageToTarget(Transform* targetTransform, float damage, const char* sourceName);
    void applyStunToTarget(Transform* targetTransform, float stunDuration, const char* sourceName);

private:
    EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
};