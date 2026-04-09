#pragma once

#include "ScriptAPI.h"

class PlayerController : public Script
{
    DECLARE_SCRIPT(PlayerController)

public:
    explicit PlayerController(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    void onAfterDeserialize() override;

public:
    float m_moveSpeed = 3.5f;
    float m_shiftMultiplier = 2.0f;
    float m_turnSpeedDegPerSec = 720.0f;

    int m_playerIndex = 0;

    bool m_constrainToNavMesh = true;
    Vector3 m_navExtents = Vector3(2.0f, 4.0f, 2.0f);

    // Animations
    std::string m_idleStateName = "Idle";
    std::string m_runStateName = "Run";

    float m_animationTransitionTime = 0.15f;

private:
    Vector3 m_initialRotationOffset = Vector3(0.0f, 0.0f, 0.0f);

    float m_currentYawDeg = 0.0f;
    bool m_yawInitialized = false;

private:
    Vector3 readMoveDirection() const;
    void applyFacingFromDirection(GameObject* owner, const Vector3& direction, float dt);
    void applyTranslation(GameObject* owner, const Vector3& direction, float dt, bool shiftHeld) const;

    static float moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta);
    static float wrapAngleDegrees(float angle);

    // Animations 
    void updateLocomotionAnimation(GameObject* owner, bool isMoving, bool isRunning) const;
    static bool isZeroMovement(const Vector3& direction);
};