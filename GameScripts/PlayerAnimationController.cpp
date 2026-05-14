#include "pch.h"
#include "PlayerAnimationController.h"

IMPLEMENT_SCRIPT_FIELDS(PlayerAnimationController,
    SERIALIZED_STRING(m_idleStateName, "Idle state name"),
    SERIALIZED_STRING(m_moveStateName, "Move state name"),
    SERIALIZED_STRING(m_dashStateName, "Dash state name"),
    SERIALIZED_STRING(m_attackStateName, "Attack state name"),
    SERIALIZED_STRING(m_damagedStateName, "Damaged state name"),
    SERIALIZED_STRING(m_downedStateName, "Downed state name"),
    SERIALIZED_STRING(m_deathStateName, "Death state name"),
    SERIALIZED_FLOAT(m_defaultBlendTime, "Default blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_attackBlendTime, "Attack blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_damagedBlendTime, "Damaged blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_downedBlendTime, "Downed blend time", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_deathBlendTime, "Death blend time", 0.0f, 2.0f, 0.01f)
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
    else if (m_damagedRequested)
    {
        desiredState = AnimState::Damaged;
        blendTime = m_damagedBlendTime;
    }
    else if (m_attackRequested)
    {
        desiredState = AnimState::Attack;
        blendTime = m_attackBlendTime;
    }
    else if (m_isDashing)
    {
        desiredState = AnimState::Dash;
    }
    else if (m_isMoving)
    {
        desiredState = AnimState::Move;
    }
    else
    {
        desiredState = AnimState::Idle;
    }

    if (desiredState != m_currentState)
    {
        if (playAnimState(desiredState, blendTime))
        {
            m_currentState = desiredState;
        }
    }

    m_attackRequested = false;
    m_damagedRequested = false;
}

void PlayerAnimationController::setMoving(bool moving)
{
    m_isMoving = moving;
}

void PlayerAnimationController::setDashing(bool dashing)
{
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
    const char* stateName = nullptr;

    switch (state)
    {
    case AnimState::Idle:    stateName = m_idleStateName.c_str(); break;
    case AnimState::Move:    stateName = m_moveStateName.c_str(); break;
    case AnimState::Dash:    stateName = m_dashStateName.c_str(); break;
    case AnimState::Attack:  stateName = m_attackStateName.c_str(); break;
    case AnimState::Damaged: stateName = m_damagedStateName.c_str(); break;
    case AnimState::Downed: stateName = m_downedStateName.c_str(); break;
    case AnimState::Death:   stateName = m_deathStateName.c_str(); break;
    default: return false;
    }

    if (!stateName || stateName[0] == '\0')
    {
        Debug::warn("PlayerAnimationController on '%s' has empty animation state name.", GameObjectAPI::getName(m_owner));
        return false;
    }

    const bool played = AnimationAPI::playState(m_animationComponent, stateName, blendTime);

    if (!played)
    {
        Debug::warn("PlayerAnimationController on '%s' could not play state '%s'.", GameObjectAPI::getName(m_owner), stateName);
    }

    return played;
}

IMPLEMENT_SCRIPT(PlayerAnimationController)