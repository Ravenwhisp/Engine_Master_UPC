#include "pch.h"
#include "PlayerAnimationController.h"

namespace
{
    std::string trimmed(const std::string& value)
    {
        const size_t start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        const size_t end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }
}

IMPLEMENT_SCRIPT_FIELDS(PlayerAnimationController,
    SERIALIZED_STRING(m_idleStateName, "Idle state name"),
    SERIALIZED_STRING(m_moveStateName, "Move state name"),
    SERIALIZED_STRING(m_dashStateName, "Dash state name"),
    SERIALIZED_STRING(m_attackStateName, "Attack state name"),
    SERIALIZED_STRING_VECTOR(m_damagedStateNames, "Damaged state names"),
    SERIALIZED_STRING(m_downedStateName, "Downed state name"),
    SERIALIZED_STRING(m_deathStateName, "Death state name"),
    SERIALIZED_FLOAT(m_defaultBlendTime, "Default blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_attackBlendTime, "Attack blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_dashBlendTime, "Dash blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_damagedBlendTime, "Damaged blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_downedBlendTime, "Downed blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_deathBlendTime, "Death blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_dashAnimDuration, "Dash anim duration", 0.0f, 3.0f, 0.01f),
    SERIALIZED_FLOAT(m_dashAnimStartPct, "Dash anim start %", 0.0f, 0.9f, 0.01f)
)

PlayerAnimationController::PlayerAnimationController(GameObject* owner)
    : Script(owner)
{
}

void PlayerAnimationController::Start()
{
	m_animationComponent = findAnimationComponent();
}

void PlayerAnimationController::Update()
{
	if (!m_animationComponent)
	{
		return;
	}

    const float dt = Time::getDeltaTime();
    if (m_damagedHoldTimer > 0.0f) m_damagedHoldTimer -= dt;
    if (m_recoveryHoldTimer > 0.0f) m_recoveryHoldTimer -= dt;
    if (m_chargeReleaseTimer > 0.0f) m_chargeReleaseTimer -= dt;
    if (m_dashHoldTimer > 0.0f) m_dashHoldTimer -= dt;

    const bool freshDamage = m_damagedRequested && !m_isDead && !m_isDowned;
    if (freshDamage)
    {
        m_currentDamagedState = pickDamagedState();
    }

    AnimState desiredState = AnimState::Idle;
    float blendTime = m_defaultBlendTime;

    if (m_isDead)
    {
        desiredState = AnimState::Death;
        blendTime = m_deathBlendTime;
    }
    else if (m_isDowned)
    {
        desiredState = AnimState::Downed;
        blendTime = m_downedBlendTime;
    }
    else if (freshDamage || m_damagedHoldTimer > 0.0f)
    {
        desiredState = AnimState::Damaged;
        blendTime = m_damagedBlendTime;
    }
    else if (m_chargeHoldActive || m_chargeReleaseTimer > 0.0f)
    {
        desiredState = AnimState::ChargeHold;
        blendTime = m_chargeHoldBlend;
    }
    else if (m_attackRequested || m_hasAttackOverride)
    {
        desiredState = AnimState::Attack;
        blendTime = m_attackBlendTime;
    }
    else if (m_isDashing || m_dashHoldTimer > 0.0f)
    {
        desiredState = AnimState::Dash;
        blendTime = m_dashBlendTime;
    }
    else if (m_isMoving)
    {
        desiredState = AnimState::Move;
    }
    else if (m_recoveryHoldTimer > 0.0f)
    {
        desiredState = AnimState::Recovery;
        blendTime = m_recoveryBlend;
    }
    else
    {
        desiredState = AnimState::Idle;
    }

    const bool forceReplay = (freshDamage && desiredState == AnimState::Damaged)
                          || (m_dashJustStarted && desiredState == AnimState::Dash);
    if (desiredState != m_currentState || forceReplay)
    {
        if (m_chargeHoldPaused && desiredState != AnimState::ChargeHold)
        {
            AnimationAPI::play(m_animationComponent);
            m_chargeHoldPaused = false;
        }

        if (playAnimState(desiredState, blendTime))
        {
            m_currentState = desiredState;

            if (desiredState == AnimState::Dash)
            {
                const float target = m_dashAnimDuration > 0.01f ? m_dashAnimDuration : 0.5f;
                const float dur = AnimationAPI::getPlaybackDuration(m_animationComponent);
                const float startPct = m_dashAnimStartPct < 0.0f ? 0.0f : (m_dashAnimStartPct > 0.9f ? 0.9f : m_dashAnimStartPct);
                const float span = dur * (1.0f - startPct);

                // Skip the clip's anticipation so the dash motion reads from the first frame.
                if (dur > 0.0001f && startPct > 0.0f)
                {
                    AnimationAPI::setPlaybackTime(m_animationComponent, dur * startPct);
                }
                if (span > 0.0001f)
                {
                    AnimationAPI::setSpeedMultiplier(m_animationComponent, span / target);
                }
                m_dashHoldTimer = target;
            }

            if (desiredState == AnimState::Damaged)
            {
                const float dur = AnimationAPI::getPlaybackDuration(m_animationComponent);
                m_damagedHoldTimer = dur > 0.0f ? dur : 0.3f;
            }

            if (desiredState != AnimState::Recovery)
            {
                m_recoveryHoldTimer = 0.0f;
            }
        }
    }

    // While charging, scrub the windup to the charge progress (playback frozen, driven manually)
    if (m_currentState == AnimState::ChargeHold && m_chargeHoldActive)
    {
        if (!m_chargeHoldPaused)
        {
            AnimationAPI::pause(m_animationComponent);
            m_chargeHoldPaused = true;
        }
        const float dur = AnimationAPI::getPlaybackDuration(m_animationComponent);
        if (dur > 0.0001f)
        {
            AnimationAPI::setPlaybackTime(m_animationComponent, m_chargeProgress * m_chargeHoldPausePct * dur);
        }
    }

    m_attackRequested = false;
    m_damagedRequested = false;
    m_dashJustStarted = false;
}

void PlayerAnimationController::setMoving(bool moving)
{
    m_isMoving = moving;
}

void PlayerAnimationController::setDashing(bool dashing, float dashDurationSeconds)
{
    if (dashing && !m_isDashing)
    {
        m_dashJustStarted = true;
    }
    if (dashing && dashDurationSeconds > 0.01f)
    {
        m_dashMoveDuration = dashDurationSeconds;
    }
    m_isDashing = dashing;
}

void PlayerAnimationController::setDowned(bool downed)
{
    m_isDowned = downed;
}

void PlayerAnimationController::setDead(bool dead)
{
    m_isDead = dead;
}

void PlayerAnimationController::requestAttack()
{
    m_attackRequested = true;
}

void PlayerAnimationController::requestDamaged()
{
    m_damagedRequested = true;
}

void PlayerAnimationController::setAttackOverride(const std::string& stateName, float blendTime, float speed)
{
    m_attackOverrideName = stateName;
    m_attackOverrideBlend = blendTime;
    m_attackOverrideSpeed = speed;
    m_hasAttackOverride = true;
}

void PlayerAnimationController::clearAttackOverride()
{
    m_hasAttackOverride = false;
}

void PlayerAnimationController::playRecovery(const std::string& stateName, float blendTime, float speed)
{
    m_hasAttackOverride = false;

    const std::string state = trimmed(stateName);
    if (state.empty() || !m_animationComponent)
    {
        return;
    }

    m_recoveryState = state;
    m_recoveryBlend = blendTime;
    m_recoverySpeed = speed;

    AnimationAPI::setSpeedMultiplier(m_animationComponent, speed);
    if (AnimationAPI::playState(m_animationComponent, state.c_str(), blendTime))
    {
        m_currentState = AnimState::Recovery;
        const float dur = AnimationAPI::getPlaybackDuration(m_animationComponent);
        const float spd = speed > 0.01f ? speed : 1.0f;
        m_recoveryHoldTimer = dur > 0.0f ? (dur / spd) : 0.3f;
    }
}

void PlayerAnimationController::beginChargeHold(const std::string& stateName, float blendTime, float speed, float pausePct)
{
    m_chargeHoldState = trimmed(stateName);
    m_chargeHoldBlend = blendTime;
    m_chargeHoldSpeed = speed;
    m_chargeHoldPausePct = pausePct;
    m_chargeHoldActive = true;
    m_chargeHoldPaused = false;
    m_chargeProgress = 0.0f;
    m_hasAttackOverride = false;
}

void PlayerAnimationController::setChargeProgress(float progress01)
{
    m_chargeProgress = progress01 < 0.0f ? 0.0f : (progress01 > 1.0f ? 1.0f : progress01);
}

void PlayerAnimationController::endChargeHold(float releaseFraction)
{
    m_chargeHoldActive = false;

    if (!m_animationComponent)
    {
        return;
    }

    const float dur = AnimationAPI::getPlaybackDuration(m_animationComponent);
    const float releaseStart = m_chargeHoldPausePct * dur;
    const float spd = m_chargeHoldSpeed > 0.01f ? m_chargeHoldSpeed : 1.0f;
    const float frac = releaseFraction < 0.05f ? 0.05f : (releaseFraction > 1.0f ? 1.0f : releaseFraction);

    // Resume and jump straight to the release frame, then play its follow-through.
    if (m_chargeHoldPaused)
    {
        AnimationAPI::play(m_animationComponent);
        m_chargeHoldPaused = false;
    }
    AnimationAPI::setSpeedMultiplier(m_animationComponent, spd);
    if (dur > 0.0001f)
    {
        AnimationAPI::setPlaybackTime(m_animationComponent, releaseStart);
    }

    // Follow-through length scales with charge (Death: full spin at any decent charge).
    const float releaseSpan = (dur - releaseStart) * frac;
    const float remaining = dur > 0.0001f ? (releaseSpan / spd) : 0.5f;
    m_chargeReleaseTimer = remaining > 0.0f ? remaining : 0.5f;
}

const std::string& PlayerAnimationController::pickDamagedState()
{
    static const std::string empty;
    if (m_damagedStateNames.empty())
    {
        return empty;
    }
    m_damagedIndex = (m_damagedIndex + 1) % static_cast<int>(m_damagedStateNames.size());
    return m_damagedStateNames[m_damagedIndex];
}

AnimationComponent* PlayerAnimationController::findAnimationComponent()
{
	m_animationComponent = AnimationAPI::getAnimationComponent(m_owner);
	if (m_animationComponent)
	{
		return m_animationComponent;
	}
	Debug::warn("CharacterAnimation on '%s' could not find an AnimationComponent on the same GameObject.", GameObjectAPI::getName(m_owner));
	return nullptr;
}

bool PlayerAnimationController::playAnimState(AnimState state, float blendTime)
{
    std::string stateName;

    switch (state)
    {
    case AnimState::Idle:    stateName = m_idleStateName; break;
    case AnimState::Move:    stateName = m_moveStateName; break;
    case AnimState::Dash:    stateName = m_dashStateName; break;
    case AnimState::Attack:  stateName = m_attackStateName; break;
    case AnimState::ChargeHold: stateName = m_chargeHoldState; break;
    case AnimState::Damaged: stateName = m_currentDamagedState; break;
    case AnimState::Recovery: stateName = m_recoveryState; break;
    case AnimState::Downed: stateName = m_downedStateName; break;
    case AnimState::Death:   stateName = m_deathStateName; break;
    default: return false;
    }

    float speed = 1.0f;
    if (state == AnimState::Attack && m_hasAttackOverride && !m_attackOverrideName.empty())
    {
        stateName = m_attackOverrideName;
        blendTime = m_attackOverrideBlend;
        speed = m_attackOverrideSpeed;
    }
    else if (state == AnimState::Recovery)
    {
        speed = m_recoverySpeed;
    }
    else if (state == AnimState::ChargeHold)
    {
        speed = m_chargeHoldSpeed;
    }

    stateName = trimmed(stateName);

    if (stateName.empty())
    {
        Debug::warn("PlayerAnimationController on '%s' has empty animation state name.", GameObjectAPI::getName(m_owner));
        return false;
    }

    AnimationAPI::setSpeedMultiplier(m_animationComponent, speed);

    const bool played = AnimationAPI::playState(m_animationComponent, stateName.c_str(), blendTime);

    if (!played)
    {
        Debug::warn("PlayerAnimationController on '%s' could not play state '%s'.", GameObjectAPI::getName(m_owner), stateName.c_str());
    }

    return played;
}

IMPLEMENT_SCRIPT(PlayerAnimationController)