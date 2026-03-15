#pragma once

#include "ScriptAPI.h"
#include "ScriptAutoRegister.h"
#include <Keyboard.h>

class Transform;

class PlayerWalk : public Script
{
    DECLARE_SCRIPT(PlayerWalk)

public:
    explicit PlayerWalk(GameObject* owner);

    void Start() override;
    void Update() override;

private:
    float m_moveSpeed = 3.5f;
    float m_shiftMultiplier = 2.0f;

    Keyboard::Keys m_keyUp = Keyboard::Keys::W;
    Keyboard::Keys m_keyLeft = Keyboard::Keys::A;
    Keyboard::Keys m_keyDown = Keyboard::Keys::S;
    Keyboard::Keys m_keyRight = Keyboard::Keys::D;
    Keyboard::Keys m_keyAscend = Keyboard::Keys::E;
    Keyboard::Keys m_keyDescend = Keyboard::Keys::Q;

    Vector3 m_initialRotationOffset = Vector3(0.0f, 0.0f, 0.0f);

    float m_turnSpeedDegPerSec = 720.0f;
    float m_currentYawDeg = 0.0f;
    bool m_yawInitialized = false;

    Vector3 readMoveDirection() const;
    void applyFacingFromDirection(Transform* transform, const Vector3& direction, float dt);
    void applyTranslation(Transform* transform, const Vector3& direction, float dt, bool shiftHeld) const;
    bool checkShiftHeld() const;

    static float moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta);
    static float wrapAngleDegrees(float angle);

    float getDeltaSecondsFromTimer() const;
};