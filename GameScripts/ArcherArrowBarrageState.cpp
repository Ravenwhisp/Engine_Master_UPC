#include "pch.h"
#include "ArcherArrowBarrageState.h"

#include "RangedEnemyController.h"
#include "ArcherAttackConfig.h"
#include "EnemyAttackExecutor.h"

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

    m_archerController->clearPath();
    m_archerController->updateCurrentTarget();

    setupUI();

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

    updateUI();

    if (m_stateTimer >= m_attackConfig->m_arrowBarrageTotalDuration)
    {
        finishArrowBarrage();
        return;
    }
}

void ArcherArrowBarrageState::OnStateExit()
{
	if (m_attackConfig)
    {
        if (m_attackConfig->m_barrageUITransform)
        {
            GameObjectAPI::setActive(m_attackConfig->m_barrageUITransform->getOwner(), false);
        }
    }

    Debug::log("[ArcherArrowBarrageState] EXIT");
}

void ArcherArrowBarrageState::lockImpactPosition()
{
    Transform* targetTransform = m_archerController->getTarget();

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

void ArcherArrowBarrageState::setupUI()
{
    if (!m_attackConfig)
    {
        return;
    }

    Transform* transform = m_attackConfig->m_barrageUITransform;
    Transform2D* transform2D = m_attackConfig->m_barrageUITransform2D;
    Transform2D* glow = m_attackConfig->m_barrageUIGlow;

    if (!transform || !transform2D || !glow)
    {
        return;
    }
	Transform2DAPI::setAlpha(transform2D, 0.0f);
	Transform2DAPI::setAlpha(glow, 0.0f);
}

void ArcherArrowBarrageState::updateUI()
{
    if (!m_attackConfig)
    {
		return;
    }

	Transform* transform = m_attackConfig->m_barrageUITransform;
	Transform2D* transform2D = m_attackConfig->m_barrageUITransform2D;
	Transform2D* glow = m_attackConfig->m_barrageUIGlow;

    if (!transform || !transform2D || !glow)
    {
        return;
	}

    if (m_stateTimer < m_attackConfig->m_arrowBarrageThrowTime)
    {
        return;
    }
    else if (m_stateTimer < m_attackConfig->m_arrowBarrageLandDelay)
    {
        GameObjectAPI::setActive(m_attackConfig->m_barrageUITransform->getOwner(), true);
        TransformAPI::setGlobalPosition(transform, m_impactPosition);
        Debug::log("[ArcherArrowBarrageState] UI position updated");

        const float t = (m_stateTimer - m_attackConfig->m_arrowBarrageThrowTime) / (m_attackConfig->m_arrowBarrageLandDelay - m_attackConfig->m_arrowBarrageThrowTime);
        Transform2DAPI::setAlpha(transform2D, t);
        Debug::log("[ArcherArrowBarrageState] UI updated, alpha: %.2f", t);
	}
    else
    {
        Transform2DAPI::setAlpha(glow, 1.0f);
        const float t = 1.0f - (m_stateTimer - m_attackConfig->m_arrowBarrageLandDelay) / (m_attackConfig->m_arrowBarrageTotalDuration - m_attackConfig->m_arrowBarrageLandDelay);
        Transform2DAPI::setAlpha(transform2D, t);
	}
}

IMPLEMENT_SCRIPT(ArcherArrowBarrageState)