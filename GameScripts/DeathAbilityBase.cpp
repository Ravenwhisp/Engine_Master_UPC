#include "pch.h"
#include "DeathAbilityBase.h"
#include "DeathCharacter.h"
#include "PlayerState.h"
#include "PlayerAnimationController.h"

DeathAbilityBase::DeathAbilityBase(GameObject* owner)
    : AbilityBase(owner)
{
}

void DeathAbilityBase::Start()
{
    m_deathChar = static_cast<DeathCharacter*>(GameObjectAPI::getScript(getOwner(), "DeathCharacter"));
    m_character = m_deathChar;

    AbilityBase::Start();

    if (m_deathChar == nullptr)
    {
        Debug::log("[DeathAbilityBase] DeathCharacter not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

void DeathAbilityBase::Update()
{
    AbilityBase::Update();

    if (m_attackStateTimer > 0.0f)
    {
        onAttackWindowUpdate();

        m_attackStateTimer -= Time::getDeltaTime();
        if (m_attackStateTimer <= 0.0f)
        {
            finishAttackWindow();
        }
    }
}

void DeathAbilityBase::beginAttackWindow(float lockDuration)
{
    m_attackStateTimer = lockDuration;
}

void DeathAbilityBase::finishAttackWindow()
{
    m_attackStateTimer = 0.0f;

    setAbilityLocked(false);

    if (m_character != nullptr)
    {
        PlayerState* playerState = m_character->getPlayerState();
        if (playerState != nullptr && playerState->isAttacking())
        {
            playerState->setState(PlayerStateType::Normal);
        }
    }

    onAttackWindowFinished();
}

void DeathAbilityBase::beginAttackPresentation()
{
    if (m_character == nullptr)
    {
        return;
    }

    PlayerState* playerState = m_character->getPlayerState();
    if (playerState != nullptr)
    {
        playerState->setState(PlayerStateType::Attacking);
        Debug::log("[DeathAbility] State -> Attacking (PlayerState found)");
    }
    else
    {
        Debug::warn("[DeathAbility] PlayerState is NULL — canMove() block will not work!");
    }

    PlayerAnimationController* animController = m_character->getAnimationController();
    if (animController != nullptr)
    {
        animController->requestAttack();
    }
}
