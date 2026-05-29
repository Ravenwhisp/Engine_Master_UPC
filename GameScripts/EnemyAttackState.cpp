#include "pch.h"
#include "EnemyAttackState.h"

#include "RangedEnemyController.h"
#include "ArcherAttackConfig.h"

#include "Damageable.h"
#include "PlayerState.h"

EnemyAttackState::EnemyAttackState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyAttackState::OnStateEnter()
{
    m_archerController = GameObjectAPI::findScript<RangedEnemyController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArcherAttackConfig>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    m_stateTimer = 0.0f;
    m_hasAppliedDamage = false;
    m_committedTarget = nullptr;

    if (!m_archerController)
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

    m_archerController->clearPath();
    m_archerController->updateCurrentTarget();
    m_committedTarget = m_archerController->getTarget();

    Debug::log("[EnemyAttackState] ENTER");
}

void EnemyAttackState::OnStateUpdate()
{
    if (!m_archerController || !m_attackConfig || !m_animation)
    {
        return;
    }

    if (m_archerController->trySendDeathTrigger(m_animation))
    {
        return;
    }

    m_archerController->faceTarget();

    m_stateTimer += Time::getDeltaTime();

    if (!m_hasAppliedDamage && m_stateTimer >= m_attackConfig->m_basicAttackWindupTime)
    {
        tryDamageTarget(m_committedTarget);
        m_hasAppliedDamage = true;
    }

    if (m_stateTimer >= m_attackConfig->m_basicAttackTotalDuration)
    {
        m_archerController->updateCurrentTarget();

        if (!m_archerController->hasTarget())
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