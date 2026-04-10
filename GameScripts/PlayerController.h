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

    // Animation triggers
    std::string m_startMoveTriggerName = "StartMove";
    std::string m_stopMoveTriggerName = "StopMove";
    std::string m_startRunTriggerName = "StartRun";
    std::string m_stopRunTriggerName = "StopRun";

private:
    Vector3 m_initialRotationOffset = Vector3(0.0f, 0.0f, 0.0f);

    float m_currentYawDeg = 0.0f;
    bool m_yawInitialized = false;

    bool m_wasMoving = false;
    bool m_wasRunning = false;
    Vector3 m_lastMoveDirection = Vector3(0.0f);

private:
    Vector3 readMoveDirection() const;
    void applyFacingFromDirection(GameObject* owner, const Vector3& direction, float dt);
    void applyTranslation(GameObject* owner, const Vector3& direction, float dt, bool shiftHeld) const;

    void updateLocomotionAnimation(GameObject* owner, bool isMoving, bool isRunning);
    void sendAnimationTrigger(GameObject* owner, const std::string& triggerName) const;

    static float moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta);
    static float wrapAngleDegrees(float angle);
    static bool isZeroMovement(const Vector3& direction);
};