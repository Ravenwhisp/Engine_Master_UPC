#include "pch.h"
#include "ArcherArrowBarrageState.h"

#include "RangedEnemyController.h"
#include "ArcherAttackConfig.h"
#include "EnemyAttackExecutor.h"
#include "ArcherUI.h"

ArcherArrowBarrageState::ArcherArrowBarrageState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArcherArrowBarrageState::OnStateEnter()
{
    m_archerController = GameObjectAPI::findScript<RangedEnemyController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArcherAttackConfig>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());
    m_archerUI = GameObjectAPI::findScript<ArcherUI>(getOwner());

    m_stateTimer = 0.0f;
    m_impactPosition = Vector3(0.0f, 0.0f, 0.0f);

    m_hasLockedImpactPosition = false;
    m_hasAppliedImpact = false;

    if (!m_archerController)
    {
        Debug::error("[ArcherArrowBarrageState] RangedEnemyController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[ArcherArrowBarrageState] ArcherAttackConfig not found.");
        return;
    }

    if (!m_attackExecutor)
    {
        Debug::error("[ArcherArrowBarrageState] EnemyAttackExecutor not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ArcherArrowBarrageState] AnimationComponent not found.");
        return;
    }

    if (!m_archerUI)
    {
        Debug::error("[ArcherArrowBarrageState] ArcherUI not found.");
        return;
    }

    m_archerController->clearPath();
    m_archerController->resetRepathTimer();

    m_archerController->updateCurrentTarget();

    m_archerUI->setupArrowBarrageUI(m_attackConfig->m_arrowBarrageRadius);

    Debug::log("[ArcherArrowBarrageState] ENTER");
}

void ArcherArrowBarrageState::OnStateUpdate()
{
    if (!m_archerController || !m_attackConfig || !m_attackExecutor || !m_animation)
    {
        return;
    }

    if (m_archerController->trySendDeathTrigger(m_animation))
    {
        return;
    }

    if (m_archerController->trySendStunTrigger(m_animation))
    {
        return;
    }

    m_stateTimer += Time::getDeltaTime();

    if (!m_hasLockedImpactPosition && m_stateTimer >= m_attackConfig->m_arrowBarrageThrowTime)
    {
        lockImpactPosition();
        m_hasLockedImpactPosition = true;
    }

    const float impactTime = m_attackConfig->m_arrowBarrageThrowTime + m_attackConfig->m_arrowBarrageLandDelay;

    if (m_hasLockedImpactPosition && !m_hasAppliedImpact && m_stateTimer >= impactTime)
    {
        applyImpact();
        m_hasAppliedImpact = true;
    }

    if (m_archerUI)
    {
        m_archerUI->updateArrowBarrageUI(m_stateTimer, m_impactPosition, m_attackConfig->m_arrowBarrageThrowTime, m_attackConfig->m_arrowBarrageLandDelay, m_attackConfig->m_arrowBarrageTotalDuration);
    }

    if (m_stateTimer >= m_attackConfig->m_arrowBarrageTotalDuration)
    {
        finishArrowBarrage();
        return;
    }
}

void ArcherArrowBarrageState::OnStateExit()
{
    if (m_archerUI)
    {
        m_archerUI->hideArrowBarrageUI();
    }

    Debug::log("[ArcherArrowBarrageState] EXIT");
}

void ArcherArrowBarrageState::lockImpactPosition()
{
    Transform* targetTransform = m_archerController->getCurrentTarget();

    if (!targetTransform)
    {
        Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
        if (!ownerTransform)
        {
            return;
        }

        m_impactPosition = TransformAPI::getGlobalPosition(ownerTransform);
        return;
    }

    m_impactPosition = TransformAPI::getGlobalPosition(targetTransform);

    Debug::log("[ArcherArrowBarrageState] Impact position locked: %.2f %.2f %.2f", m_impactPosition.x, m_impactPosition.y, m_impactPosition.z);
}

void ArcherArrowBarrageState::applyImpact()
{
    m_attackExecutor->applyDamageInRadius(m_impactPosition, m_attackConfig->m_arrowBarrageRadius, m_attackConfig->m_arrowBarrageDamage,"ArrowBarrage");

    Debug::log("[ArcherArrowBarrageState] Impact applied.");
}

void ArcherArrowBarrageState::finishArrowBarrage()
{
    if (!m_archerController || !m_animation)
    {
        return;
    }

    m_archerController->consumeArrowBarrageCooldown();

    AnimationAPI::sendTrigger(m_animation, "ToChase");

    Debug::log("[ArcherArrowBarrageState] Finished, Chase trigger sent");
}

IMPLEMENT_SCRIPT(ArcherArrowBarrageState)