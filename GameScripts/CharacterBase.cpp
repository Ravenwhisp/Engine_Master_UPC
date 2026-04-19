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
    Script* stateScript = GameObjectAPI::getScript(getOwner(), "PlayerState");
    m_playerState = static_cast<PlayerState*>(stateScript);

    Script* controllerScript = GameObjectAPI::getScript(getOwner(), "PlayerController");
    m_playerController = static_cast<PlayerController*>(controllerScript);

    Script* rotationScript = GameObjectAPI::getScript(getOwner(), "PlayerRotation");
    m_playerRotation = static_cast<PlayerRotation*>(rotationScript);

    Script* animationScript = GameObjectAPI::getScript(getOwner(), "PlayerAnimationController");
    m_playerAnimationController = static_cast<PlayerAnimationController*>(animationScript);

    Script* targetControllerScript = GameObjectAPI::getScript(getOwner(), "PlayerTargetController");
    m_targetController = static_cast<PlayerTargetController*>(targetControllerScript);

    Script* damageableScript = GameObjectAPI::getScript(getOwner(), "PlayerDamageable");
    m_damageable = static_cast<Damageable*>(damageableScript);

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