#include "pch.h"
#include "CharacterBase.h"

#include "PlayerState.h"
#include "PlayerController.h"
#include "PlayerRotation.h"
#include "PlayerAnimationController.h"
#include "PlayerTargetController.h"
#include "Damageable.h"

CharacterBase::CharacterBase(GameObject* owner)
    : Script(owner)
{
}

void CharacterBase::Start()
{
    m_playerState = GameObjectAPI::findScript<PlayerState>(getOwner());
    m_playerController = GameObjectAPI::findScript<PlayerController>(getOwner());
    m_playerRotation = GameObjectAPI::findScript<PlayerRotation>(getOwner());
    m_playerAnimationController = GameObjectAPI::findScript<PlayerAnimationController>(getOwner());
    m_targetController = GameObjectAPI::findScript<PlayerTargetController>(getOwner());
    m_damageable = GameObjectAPI::findScript<Damageable>(getOwner());

    if (m_playerState == nullptr)
    {
        Debug::log("[CharacterBase] PlayerState not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_playerController == nullptr)
    {
        Debug::log("[CharacterBase] PlayerController not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_playerRotation == nullptr)
    {
        Debug::log("[CharacterBase] PlayerRotation not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_playerAnimationController == nullptr)
    {
        Debug::log("[CharacterBase] PlayerAnimationController not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_targetController == nullptr)
    {
        Debug::log("[CharacterBase] PlayerTargetController not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_damageable == nullptr)
    {
        Debug::log("[CharacterBase] Damageable not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

int CharacterBase::getPlayerIndex() const
{
    if (m_playerController == nullptr)
    {
        return 0;
    }

    return m_playerController->getPlayerIndex();
}

bool CharacterBase::isDowned() const
{
    return m_playerState != nullptr && m_playerState->isDowned();
}

bool CharacterBase::isUsingAbility() const
{
    return m_playerState != nullptr && m_playerState->isUsingAbility();
}

void CharacterBase::setUsingAbility(bool value)
{
    if (m_playerState != nullptr)
    {
        m_playerState->setUsingAbility(value);
    }
}