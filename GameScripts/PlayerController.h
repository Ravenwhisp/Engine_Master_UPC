#pragma once

#include "ScriptAPI.h"

class PlayerMovement;
class PlayerRotation;
class PlayerState;

class PlayerController : public Script
{
    DECLARE_SCRIPT(PlayerController)

public:
    explicit PlayerController(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    bool getGodMode() const { return m_godMode; }
    int getPlayerIndex() const { return m_playerIndex; }
    Vector3 getMoveDirection() const;

public:
    int m_playerIndex = 0;
    bool m_godMode = false;

private:
    PlayerMovement* m_playerMovement = nullptr;
    PlayerRotation* m_playerRotation = nullptr;
    PlayerState* m_playerState = nullptr;

    Transform* m_cameraTransform = nullptr;

private:
    Vector3 readMoveDirection(const Vector2& moveAxis) const;

    PlayerMovement* findMovementScript(GameObject* owner);
    PlayerRotation* findRotationScript(GameObject* owner);
    PlayerState* findStateScript(GameObject* owner);
};