#pragma once

#include "Component.h"
#include <Keyboard.h>

class InputModule;
class Transform;
class GameObject;

class NavMeshWalk final : public Component
{
public:
    NavMeshWalk(UID id, GameObject* gameobject);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void update() override;
    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentJson) override;

    bool getNavmeshConstrain() const { return m_constrainToNavMesh; }
    void setNavmeshConstrain(bool v) { m_constrainToNavMesh = v; }

private:
    float getDeltaSecondsFromTimer() const;

    Vector3 readMoveDirection(InputModule* inputModule) const;
    bool checkShiftHeld(InputModule* inputModule) const;

    void applyFacingFromDirection(Transform* transform, const Vector3& direction, float dt);
    void applyTranslation(Transform* transform, const Vector3& direction, float dt, bool shiftHeld) const;

    static float wrapAngleDegrees(float angle);
    static float moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta);

private:
    enum class ControlScheme : int
    {
        WASD = 0,
        IJKL,
        COUNT
    };

    void applyControlScheme();
    bool drawControlSchemeCombo(ControlScheme& scheme);
    const char* controlSchemeToString(ControlScheme scheme);

private:
    ControlScheme m_controlScheme = ControlScheme::WASD;

    float m_moveSpeed = 5.0f;
    float m_shiftMultiplier = 2.0f;
    float m_turnSpeedDegPerSec = 360.0f;

    Vector3 m_initialRotationOffset = Vector3::Zero;

    bool  m_yawInitialized = false;
    float m_currentYawDeg = 0.0f;

    Keyboard::Keys m_keyUp = Keyboard::Keys::W;
    Keyboard::Keys m_keyDown = Keyboard::Keys::S;
    Keyboard::Keys m_keyLeft = Keyboard::Keys::A;
    Keyboard::Keys m_keyRight = Keyboard::Keys::D;

    // NavMesh constraint
    bool  m_constrainToNavMesh = true;
    float m_navExtents[3] = { 2.0f, 4.0f, 2.0f }; // X,Y,Z
};