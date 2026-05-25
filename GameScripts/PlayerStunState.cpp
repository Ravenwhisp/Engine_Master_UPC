#include "pch.h"
#include "PlayerStunState.h"

#include "PlayerState.h"
#include "PlayerMovement.h"

PlayerStunState::PlayerStunState(GameObject* owner)
    : Script(owner)
{
}

void PlayerStunState::Start()
{
    m_playerState = GameObjectAPI::findScript<PlayerState>(getOwner());

    if (!m_playerState)
    {
        Debug::warn("PlayerStunState on '%s' could not find PlayerState on the same GameObject.", GameObjectAPI::getName(getOwner()));
    }

    m_playerMovement = GameObjectAPI::findScript<PlayerMovement>(getOwner());
}

void PlayerStunState::Update()
{
    if (!m_playerState)
    {
        return;
    }

    if (!m_playerState->isStunned())
    {
        return;
    }

    m_stunTimer -= Time::getDeltaTime();

    if (m_stunTimer > 0.0f)
    {
        return;
    }

    clearStun();
}

void PlayerStunState::enterStun(float durationSeconds)
{
    if (!m_playerState)
    {
        return;
    }

    if (durationSeconds <= 0.0f)
    {
        return;
    }

    // Downed has higher priority.
    if (m_playerState->isDowned())
    {
        return;
    }

    // Stun does not override itself, if the player is already stunned, ignore new stun attempts
    if (m_playerState->isStunned())
    {
        return;
    }

    m_stunTimer = durationSeconds;

    m_playerState->setState(PlayerStateType::Stunned);
    m_playerState->setUsingAbility(false);

    if (m_playerMovement)
    {
        m_playerMovement->setMoving(false);
    }

    Debug::log("%s stunned for %.2f seconds.", GameObjectAPI::getName(getOwner()), durationSeconds);
}

void PlayerStunState::clearStun()
{
    if (!m_playerState)
    {
        return;
    }

    m_stunTimer = 0.0f;

    if (m_playerState->isStunned())
    {
        m_playerState->setState(PlayerStateType::Normal);
        m_playerState->setUsingAbility(false);

        Debug::log("%s stun ended.", GameObjectAPI::getName(getOwner()));
    }
}

bool PlayerStunState::isStunned() const
{
    return m_playerState && m_playerState->isStunned();
}

IMPLEMENT_SCRIPT(PlayerStunState)