#include "pch.h"
#include "LyrielAbilityBase.h"

#include "LyrielCharacter.h"
#include "PlayerRotation.h"
#include "PlayerState.h"
#include "PlayerAnimationController.h"

LyrielAbilityBase::LyrielAbilityBase(GameObject* owner)
    : AbilityBase(owner)
{
}

void LyrielAbilityBase::Start()
{
    AbilityBase::Start();

    m_lyriel = dynamic_cast<LyrielCharacter*>(GameObjectAPI::getScript(getOwner(), "LyrielCharacter"));
    m_character = m_lyriel;

    if (m_lyriel == nullptr)
    {
        Debug::log("[LyrielAbilityBase] LyrielCharacter not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

void LyrielAbilityBase::Update()
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

Transform* LyrielAbilityBase::findArrowSpawnTransform() const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr || m_lyriel == nullptr)
    {
        return nullptr;
    }

    if (!m_lyriel->m_arrowSpawnChildName.empty())
    {
        Transform* spawnTransform = TransformAPI::findChildByName(
            ownerTransform,
            m_lyriel->m_arrowSpawnChildName.c_str());

        if (spawnTransform != nullptr)
        {
            return spawnTransform;
        }
    }

    return ownerTransform;
}

void LyrielAbilityBase::faceDirection(const Vector3& direction)
{
    if (m_character == nullptr)
    {
        return;
    }

    PlayerRotation* playerRotation = m_character->getPlayerRotation();
    if (playerRotation == nullptr)
    {
        return;
    }

    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatDirection.Normalize();
    playerRotation->applyFacingFromDirection(getOwner(), flatDirection, Time::getDeltaTime());
}

Vector3 LyrielAbilityBase::getFallbackFacingDirection() const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        return Vector3::Zero;
    }

    Vector3 forward = TransformAPI::getForward(ownerTransform);
    forward.y = 0.0f;

    if (forward.LengthSquared() <= 0.0001f)
    {
        return Vector3::Zero;
    }

    forward.Normalize();
    return forward;
}

void LyrielAbilityBase::beginAttackWindow(float lockDuration)
{
    m_attackStateTimer = lockDuration;
}

void LyrielAbilityBase::finishAttackWindow()
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

void LyrielAbilityBase::beginAttackPresentation()
{
    if (m_character == nullptr)
    {
        return;
    }

    PlayerState* playerState = m_character->getPlayerState();
    if (playerState != nullptr)
    {
        playerState->setState(PlayerStateType::Attacking);
    }

    PlayerAnimationController* animationController = m_character->getAnimationController();
    if (animationController != nullptr)
    {
        animationController->requestAttack();
    }
}