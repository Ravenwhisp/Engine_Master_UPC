#pragma once

#include "ScriptAPI.h"

class PlayerState;
class PlayerMovement;

class PlayerStunState : public Script
{
    DECLARE_SCRIPT(PlayerStunState)

public:
    explicit PlayerStunState(GameObject* owner);

    void Start() override;
    void Update() override;

public:
    void enterStun(float durationSeconds);
    void clearStun();

    bool isStunned() const;
    float getRemainingStunTime() const { return m_stunTimer; }

private:
    PlayerState* m_playerState = nullptr;
    PlayerMovement* m_playerMovement = nullptr;

    float m_stunTimer = 0.0f;
};