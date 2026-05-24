#pragma once

#include "ScriptAPI.h"

class ArthurDetectionAggro;
class PlayerStunState;

class ArthurAttackExecutor : public Script
{
    DECLARE_SCRIPT(ArthurAttackExecutor)

public:
    explicit ArthurAttackExecutor(GameObject* owner);

    void Start() override;

    void applyDamageInRadius(const Vector3& center, float radius, float damage, const char* sourceName);
    void applyDamageAndStunInRadius(const Vector3& center, float radius, float damage, float stunDuration, const char* sourceName);
    void applyDamageInCone(const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, const char* sourceName);

    bool tryDamageTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, const char* sourceName);
    void tryDamageAndStunTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, float stunDuration, const char* sourceName);
    bool tryDamageTargetInCone(Transform* targetTransform, const Vector3& center, const Vector3& direction, float range, float halfAngleDegrees, float damage, const char* sourceName);


private:
    bool applyDamageToTarget(Transform* targetTransform, float damage, const char* sourceName);
    void applyStunToTarget(Transform* targetTransform, float stunDuration, const char* sourceName);

private:
    ArthurDetectionAggro* m_arthurDetectionAggro = nullptr;
};