#include "pch.h"
#include "PlayerDamageable.h"

#include "PlayerDownState.h"
#include "PlayerAnimationController.h"

PlayerDamageable::PlayerDamageable(GameObject* owner)
    : Damageable(owner)
{
}

void PlayerDamageable::Start()
{
    Damageable::Start();

    m_playerAnimationController = GameObjectAPI::findScript<PlayerAnimationController>(m_owner);

    if (m_playerAnimationController == nullptr)
    {
        Debug::warn("%s has PlayerDamageable but no PlayerAnimationController.", GameObjectAPI::getName(m_owner));
    }
}

void PlayerDamageable::onDamaged(float amount)
{
    Damageable::onDamaged(amount);

    if (m_playerAnimationController != nullptr)
    {
        m_playerAnimationController->requestDamaged();
    }
}

void PlayerDamageable::onHpDepleted()
{
    PlayerDownState* downState = GameObjectAPI::findScript<PlayerDownState>(m_owner);

    if (downState)
    {
        downState->enterDownState();

        if (m_playerAnimationController != nullptr)
        {
            m_playerAnimationController->setDowned(true);
        }

        return;
    }

    Debug::warn("%s has PlayerDamageable but no PlayerDownState. Falling back to kill.", GameObjectAPI::getName(m_owner));
    Damageable::onHpDepleted();
}

void PlayerDamageable::onDeath()
{
    Damageable::onDeath();

    if (m_playerAnimationController != nullptr)
    {
        m_playerAnimationController->setDead(true);
    }
}

void PlayerDamageable::onRevive()
{
    Damageable::onRevive();

    if (m_playerAnimationController != nullptr)
    {
        m_playerAnimationController->setDead(false);
        m_playerAnimationController->setDowned(false);
    }
}

IMPLEMENT_SCRIPT(PlayerDamageable)