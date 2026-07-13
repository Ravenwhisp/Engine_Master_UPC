#include "pch.h"
#include "DeathAbilityBase.h"

#include "DeathCharacter.h"
#include "PlayerState.h"
#include "DeathConfig.h"

DeathAbilityBase::DeathAbilityBase(GameObject* owner)
    : AbilityBase(owner)
{
}

void DeathAbilityBase::Start()
{
    AbilityBase::Start();

    m_deathCharacter = dynamic_cast<DeathCharacter*>(m_character);
}

void DeathAbilityBase::releaseComboMoveLock()
{
    m_movementLockedForCombo = false;

    // Another ability may still be holding the lock, e.g. basic attack window still active.
    // Leave PlayerState alone. That ability's finishAttackWindow will release it.
    if (m_character != nullptr && m_character->isUsingAbility())
    {
        return;
    }

    PlayerState* playerState = m_character != nullptr ? m_character->getPlayerState() : nullptr;
    if (playerState != nullptr && playerState->isRecoveringAttack())
    {
        playerState->setState(PlayerStateType::Normal);
    }
}
