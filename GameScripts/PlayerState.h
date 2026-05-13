#pragma once

#include "ScriptAPI.h"

enum class PlayerStateType
{
    Normal = 0,
    AttackRecovery,
    Downed
};

class PlayerState : public Script
{
    DECLARE_SCRIPT(PlayerState)

public:
    explicit PlayerState(GameObject* owner);

    PlayerStateType getState() const { return static_cast<PlayerStateType>(m_state); }
    void setState(PlayerStateType state);

    bool isDowned() const { return getState() == PlayerStateType::Downed; }
    bool isRecoveringAttack() const { return getState() == PlayerStateType::AttackRecovery; }
    bool canMove() const { return getState() == PlayerStateType::Normal; }

    bool isUsingAbility() const;
    void setUsingAbility(bool value);

private:
    int m_state = static_cast<int>(PlayerStateType::Normal);

    bool m_isUsingAbility = false;
};