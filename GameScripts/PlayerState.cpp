#include "pch.h"
#include "PlayerState.h"

PlayerState::PlayerState(GameObject* owner)
    : Script(owner)
{
}

void PlayerState::setState(PlayerStateType state)
{
    m_state = static_cast<int>(state);
}

bool PlayerState::isUsingAbility() const
{
    return m_isUsingAbility;
}

void PlayerState::setUsingAbility(bool value)
{
    m_isUsingAbility = value;
}

bool PlayerState::canUseAbilities() const
{
    return getState() != PlayerStateType::Downed && getState() != PlayerStateType::Stunned;
}

IMPLEMENT_SCRIPT(PlayerState)