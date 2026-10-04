#include "pch.h"
#include "AbilityBase.h"

#include "CharacterBase.h"
#include "PlayerState.h"
#include "PlayerAnimationController.h"
#include "CharacterAnimations.h"
#include "CharacterUI.h"

static const char* abilityUISlotNames[] =
{
    "Basic Attack",
    "Charged Attack",
    "Ability",
    "Dash"
};

constexpr int abilityUISlotCount = 4;

static AttackAnimId animIdForSlot(int uiSlot)
{
    switch (static_cast<AbilityUISlot>(uiSlot))
    {
    case AbilityUISlot::ChargedAttack: return AttackAnimId::Charged;
    case AbilityUISlot::Ability:       return AttackAnimId::Special;
    default:                           return AttackAnimId::Basic;
    }
}

IMPLEMENT_SCRIPT_FIELDS(AbilityBase,
    SERIALIZED_ENUM_INT(m_uiSlot, "UI Slot", abilityUISlotNames, abilityUISlotCount),
    SERIALIZED_FLOAT(m_moveLockDuration, "Move Lock Duration", 0.0f, 2.0f, 0.05f)
)

AbilityBase::AbilityBase(GameObject* owner)
    : Script(owner)
{
}

void AbilityBase::Start()
{
    m_character = GameObjectAPI::findScript<CharacterBase>(getOwner());

    if (m_character == nullptr)
    {
        Debug::warn("[AbilityBase] CharacterBase not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    m_characterUI = GameObjectAPI::findScript<CharacterUI>(getOwner());

    if (m_characterUI == nullptr)
    {
        Debug::warn("[AbilityBase] CharacterUI not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    m_animComp = AnimationAPI::getAnimationComponent(getOwner());
    m_attackAnims = GameObjectAPI::findScript<CharacterAnimations>(getOwner());
}

void AbilityBase::Update()
{
	float dt = Time::getDeltaTime();

    updateCooldown(dt);
	updateAttackWindow(dt);
    updateUI();
}

void AbilityBase::tryAbility()
{
    if (!canStartAbility())
    {
        return;
    }

    if (m_character != nullptr)
    {
        AbilityBase* active = m_character->getActiveAbility();
        if (active != nullptr)
        {
            active->cancelAbility();
        }
    }

    startAbility();
}

void AbilityBase::updateCooldown(float dt)
{
    if (m_cooldownTimer <= 0.0f)
    {
        return;
    }

    m_cooldownTimer -= dt;

    if (m_cooldownTimer < 0.0f)
    {
        m_cooldownTimer = 0.0f;

        const AbilityUISlot slot = static_cast<AbilityUISlot>(m_uiSlot);
        m_characterUI->hideAbilityCooldown(slot);
    }
}

void AbilityBase::updateUI()
{
    if (!m_characterUI)
    {
        return;
    }

    const float cooldown = getCooldown();

    if (m_cooldownTimer <= 0.0f || cooldown <= 0.0001f)
    {
        return;
    }

    const AbilityUISlot slot = static_cast<AbilityUISlot>(m_uiSlot);
    m_characterUI->updateAbilityCooldown(slot, m_cooldownTimer / cooldown);
}

void AbilityBase::updateHUDControlScale(Transform2D* hudControl, bool isPressed, float dt)
{
    if (!hudControl)
        return;

    const float targetScale = isPressed ? 0.8f : 1.0f;
    const float currentScale = Transform2DAPI::getScale(hudControl).x;
    const float scale = MathAPI::moveTowards(currentScale, targetScale, dt);

    Transform2DAPI::setScale(hudControl, Vector2(scale, scale));
}

void AbilityBase::reduceCooldown(float fraction)
{
    const float cooldown = getCooldown();

    if (m_cooldownTimer <= 0.0f || fraction <= 0.0f || cooldown <= 0.0f)
    {
        return;
    }

    m_cooldownTimer -= fraction * cooldown;

    if (m_cooldownTimer <= 0.0f)
    {
        m_cooldownTimer = 0.0f;

        if (m_characterUI)
        {
            m_characterUI->hideAbilityCooldown(static_cast<AbilityUISlot>(m_uiSlot));
        }

        return;
    }

    if (m_characterUI)
    {
        m_characterUI->updateAbilityCooldown(static_cast<AbilityUISlot>(m_uiSlot), m_cooldownTimer / cooldown);
    }
}

void AbilityBase::startCooldown()
{
    m_cooldownTimer = getCooldown();

    if (m_characterUI)
    {
        m_characterUI->showAbilityCooldown(static_cast<AbilityUISlot>(m_uiSlot));
        m_characterUI->updateAbilityCooldown(static_cast<AbilityUISlot>(m_uiSlot), 1.0f);
    }
}

void AbilityBase::updateAttackWindow(float dt)
{
    if (!m_attackWindowActive)
    {
        return;
    }

    onAttackWindowUpdate();

    if (!m_moveLockReleased)
    {
        m_moveLockTimer -= dt;
        if (m_moveLockTimer <= 0.0f)
        {
            m_moveLockReleased = true;
            releaseMovementLock();
        }
    }

    if (m_attackStateTimer > 0.0f)
    {
        m_attackStateTimer -= dt;
        if (m_attackStateTimer < 0.0f)
        {
            m_attackStateTimer = 0.0f;
        }
    }

    if (usesAnimHitTiming())
    {
        m_attackWindowElapsed += dt;

        const char* activeState = AnimationAPI::getActiveStateName(m_animComp);
        const bool onOurClip = (activeState != nullptr && m_curAnimState == activeState);

        if (onOurClip)
        {
            m_sawOurClip = true;

            const float duration = AnimationAPI::getPlaybackDuration(m_animComp);
            const float progress = (duration > 0.0001f)
                ? (AnimationAPI::getPlaybackTime(m_animComp) / duration)
                : 0.0f;

            if (!m_hitFired && progress >= m_curHitPct)
            {
                m_hitFired = true;
                onHitFrame();
            }

            if (progress >= m_curRecoverPct)
            {
                finishAttackWindow();
                return;
            }
        }
        else if (m_sawOurClip)
        {
            // Our clip was playing and got replaced (interrupted, or it ended and the
            // controller moved on) -> end the window instead of hanging on the safety cap.
            finishAttackWindow();
            return;
        }
        else if (m_attackStateTimer <= 0.0f)
        {
            // Clip tracking never engaged within the lock duration -> don't soft-lock.
            finishAttackWindow();
            return;
        }

        constexpr float k_animSafetyCap = 8.0f;
        if (m_attackWindowElapsed >= k_animSafetyCap)
        {
            finishAttackWindow();
        }
    }
    else
    {
        if (!m_hitFired)
        {
            m_hitFired = true;
            onHitFrame();
        }

        if (m_attackStateTimer <= 0.0f)
        {
            finishAttackWindow();
        }
    }
}

void AbilityBase::notifyAbilitySuccessfullyStarted()
{
    ++m_successfulUseCount;
}

bool AbilityBase::canStartAbility() const
{
    if (m_character == nullptr)
    {
        return false;
    }

    if (!m_isEnabled)
    {
        return false;
    }

    if (!isCooldownReady())
    {
        return false;
    }

    if (m_character->isDowned())
    {
        return false;
    }

    if (m_character->isUsingAbility())
    {
        AbilityBase* active = m_character->getActiveAbility();
        if (active == nullptr)
        {
            return false;
        }

        // The dash always cuts through. Anything else only once the active action
        // already landed its hit and its move lock expired.
        if (!canCancelOthers() && !active->isCancelable())
        {
            return false;
        }
    }

    if (!canStartSpecificAbility())
    {
        return false;
    }

    return true;
}

void AbilityBase::setAbilityLocked(bool locked) //innecesario
{
    if (m_character != nullptr)
    {
        m_character->setUsingAbility(locked);

        if (locked)
        {
            m_character->setActiveAbility(this);
        }
        else if (m_character->getActiveAbility() == this)
        {
            m_character->setActiveAbility(nullptr);
        }
    }
}

int AbilityBase::getPlayerIndex() const //innecesario
{
    if (m_character == nullptr)
    {
        return 0;
    }

    return m_character->getPlayerIndex();
}

void AbilityBase::beginAttackWindow(float lockDuration)
{
    m_attackStateTimer = lockDuration;
    m_attackWindowActive = true;
    m_hitFired = false;
    m_sawOurClip = false;
    m_attackWindowElapsed = 0.0f;
    m_moveLockTimer = m_moveLockDuration > 0.001f ? m_moveLockDuration : 0.4f;
    m_moveLockReleased = false;
}

void AbilityBase::finishAttackWindow()
{
    m_attackStateTimer = 0.0f;
    m_attackWindowActive = false;

    if (!m_hitFired)
    {
        m_hitFired = true;
        onHitFrame();
    }

    setAbilityLocked(false);

    if (m_character != nullptr)
    {
        PlayerAnimationController* animController = m_character->getAnimationController();
        if (animController != nullptr)
        {
            if (!m_curRecoveryState.empty())
            {
                animController->playRecovery(m_curRecoveryState, m_curAnimBlend, m_curAnimSpeed);
            }
            else
            {
                animController->clearAttackOverride();
            }
        }

        PlayerState* playerState = m_character->getPlayerState();
        if (playerState != nullptr && playerState->isRecoveringAttack())
        {
            playerState->setState(PlayerStateType::Normal);
        }
    }

    onAttackWindowFinished();
}

bool AbilityBase::isCancelable() const
{
    return m_attackWindowActive && m_moveLockReleased && m_hitFired;
}

void AbilityBase::releaseMovementLock()
{
    if (m_character == nullptr)
    {
        return;
    }

    PlayerState* playerState = m_character->getPlayerState();
    if (playerState != nullptr && playerState->isRecoveringAttack())
    {
        playerState->setState(PlayerStateType::Normal);
    }
}

void AbilityBase::cancelAbility()
{
    const bool wasActive = m_attackWindowActive;

    m_attackStateTimer = 0.0f;
    m_attackWindowActive = false;
    m_moveLockTimer = 0.0f;
    m_moveLockReleased = true;

    // Only the animation is cancelled. A committed attack still lands its hit, otherwise
    // the ability is lost while its cooldown already started.
    if (wasActive && !m_hitFired)
    {
        m_hitFired = true;
        onHitFrame();
    }

    setAbilityLocked(false);

    if (m_character != nullptr)
    {
        PlayerAnimationController* animController = m_character->getAnimationController();
        if (animController != nullptr)
        {
            animController->clearAttackOverride();
        }

        PlayerState* playerState = m_character->getPlayerState();
        if (playerState != nullptr && playerState->isRecoveringAttack())
        {
            playerState->setState(PlayerStateType::Normal);
        }
    }

    onAttackWindowFinished();
}

bool AbilityBase::usesAnimHitTiming() const
{
    return m_animComp != nullptr && !m_curAnimState.empty();
}

void AbilityBase::resolveCurrentAttackAnim()
{
    if (m_attackAnims != nullptr)
    {
        const AttackAnimInfo info = m_attackAnims->resolve(animIdForSlot(m_uiSlot), getAttackVariant());
        m_curAnimState = info.stateName;
        m_curAnimSpeed = info.speed;
        m_curAnimBlend = info.blendIn;
        m_curHitPct = info.actionPct;
        m_curRecoverPct = info.recoverPct;
        m_curRecoveryState = info.recoveryState;
    }
    else
    {
        m_curAnimState.clear();
        m_curRecoveryState.clear();
    }
}

void AbilityBase::beginAttackPresentation()
{
    if (m_character == nullptr)
    {
        return;
    }

    PlayerState* playerState = m_character->getPlayerState();
    if (playerState != nullptr)
    {
        if (playerState->isDowned())
        {
            return;
        }

        playerState->setState(PlayerStateType::AttackRecovery);
    }

    resolveCurrentAttackAnim();

    PlayerAnimationController* animController = m_character->getAnimationController();
    if (animController != nullptr)
    {
        if (!m_curAnimState.empty())
        {
            animController->setAttackOverride(m_curAnimState, m_curAnimBlend, m_curAnimSpeed);
        }
        animController->requestAttack();
    }
}

Vector3 AbilityBase::computeCameraRelativeAimDirection(float deadzoneSq) const //no me gustan transforms aqui
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        return Vector3::Zero;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);

    return Input::getAimDirection(ownerPosition, getPlayerIndex(), deadzoneSq);
}

Vector3 AbilityBase::getFallbackFacingDirection() const
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

IMPLEMENT_SCRIPT(AbilityBase)
