#pragma once

#include "ScriptAPI.h"

class AelorinDetectionAggro;

class AelorinAttackExecutor : public Script
{
    DECLARE_SCRIPT(AelorinAttackExecutor)

public:
    explicit AelorinAttackExecutor(GameObject* owner);

    void Start() override;

    bool isValidPlayerTarget(Transform* targetTransform) const;
    bool isValidDamageTarget(Transform* targetTransform) const;

    // Radius Damage
    void applyDamageInRadius(const Vector3& center, float radius, float damage, const char* sourceName);
    bool tryDamageTargetInRadius(Transform* targetTransform, const Vector3& center, float radius, float damage, const char* sourceName);

    // Beam Damage
    void applyDamageInBeam(const Vector3& origin, const Vector3& direction, float length, float width, float damage, const char* sourceName);
    bool tryDamageTargetInBeam(Transform* targetTransform, const Vector3& origin, const Vector3& direction, float length, float halfWidth, float damage, const char* sourceName);
    
private:
    bool applyDamageToTarget(Transform* targetTransform, float damage, const char* sourceName);

private:
    AelorinDetectionAggro* m_detectionAggro = nullptr;
};