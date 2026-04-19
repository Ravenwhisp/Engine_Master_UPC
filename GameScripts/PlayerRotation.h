#pragma once

#include "ScriptAPI.h"

class PlayerRotation : public Script
{
    DECLARE_SCRIPT(PlayerRotation)

public:
    explicit PlayerRotation(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    void onAfterDeserialize() override;

    void applyFacingFromDirection(GameObject* owner, const Vector3& direction, float dt);

public:
    float m_turnSpeedDegPerSec = 720.0f;

private:
    Vector3 m_initialRotationOffset = Vector3(0.0f, 0.0f, 0.0f);

    float m_currentYawDeg = 0.0f;
    bool m_yawInitialized = false;

private:
    static float moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta);
    static float wrapAngleDegrees(float angle);

};