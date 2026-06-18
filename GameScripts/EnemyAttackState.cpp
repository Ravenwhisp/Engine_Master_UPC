#include "pch.h"
#include "EnemyAttackState.h"

#include "EnemyBaseController.h"
#include "EnemyBaseAttackConfig.h"

#include "Damageable.h"
#include "PlayerState.h"

EnemyAttackState::EnemyAttackState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyAttackState::OnStateEnter()
{
    m_controller = GameObjectAPI::findScript<EnemyBaseController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<EnemyBaseAttackConfig>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    m_stateTimer = 0.0f;
    m_hasAppliedDamage = false;
    m_committedTarget = nullptr;

    if (!m_controller)
    {
        Debug::error("[EnemyAttackState] RangedEnemyController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[EnemyAttackState] ArcherAttackConfig not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[EnemyAttackState] AnimationComponent not found.");
        return;
    }

    m_controller->clearPath();
    m_controller->resetRepathTimer();

    m_controller->updateCurrentTarget();
    m_committedTarget = m_controller->getCurrentTarget();

    Debug::log("[EnemyAttackState] ENTER");
}

void EnemyAttackState::OnStateUpdate()
{
    if (!m_controller || !m_attackConfig || !m_animation)
    {
        return;
    }

    if (m_controller->trySendDeathTrigger(m_animation))
    {
        return;
    }

    if (m_controller->trySendStunTrigger(m_animation))
    {
        return;
    }

    m_controller->faceCurrentTarget();

    m_stateTimer += Time::getDeltaTime();

    if (!m_hasAppliedDamage && m_stateTimer >= m_attackConfig->m_basicAttackWindupTime)
    {
        tryDamageTarget(m_committedTarget);
        m_hasAppliedDamage = true;
    }

    if (m_stateTimer >= m_attackConfig->m_basicAttackTotalDuration)
    {
        m_controller->updateCurrentTarget();

        if (!m_controller->hasValidTarget())
        {
            AnimationAPI::sendTrigger(m_animation, "ToIdle");
            Debug::log("[EnemyAttackState] Attack finished, Idle trigger sent");
        }
        else
        {
            AnimationAPI::sendTrigger(m_animation, "ToChase");
            Debug::log("[EnemyAttackState] Attack finished, Chase trigger sent");
        }

        return;
    }
}

void EnemyAttackState::OnStateExit()
{
    Debug::log("[EnemyAttackState] EXIT");
}

void EnemyAttackState::tryDamageTarget(Transform* targetTransform)
{
    if (!m_attackConfig)
    {
        return;
    }

    if (!targetTransform)
    {
        return;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);
    if (!targetObject)
    {
        return;
    }

    PlayerState* playerState = GameObjectAPI::findScript<PlayerState>(targetObject);
    if (playerState && playerState->isDowned())
    {
        return;
    }

    Damageable* damageable = GameObjectAPI::findScript<Damageable>(targetObject);
    if (!damageable)
    {
        return;
    }

    damageable->takeDamage(m_attackConfig->m_basicAttackDamage);

    Debug::log("[EnemyAttackState] Damaged '%s' for %.2f.", GameObjectAPI::getName(targetObject), m_attackConfig->m_basicAttackDamage);
}

IMPLEMENT_SCRIPT(EnemyAttackState)