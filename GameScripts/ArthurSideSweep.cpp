#include "pch.h"
#include "ArthurSideSweep.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "EnemyAttackExecutor.h"

IMPLEMENT_SCRIPT_FIELDS(ArthurSideSweep,
    SERIALIZED_INT(m_sweepSide, "Sweep Side")
)

ArthurSideSweep::ArthurSideSweep(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurSideSweep::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    m_stateTimer = 0.0f;
    m_hasAppliedHit = false;

    if (!m_arthurController)
    {
        Debug::error("[ArthurSideSweep] ArthurBossController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[ArthurSideSweep] ArthurAttackConfig not found.");
        return;
    }

    if (!m_attackExecutor)
    {
        Debug::error("[ArthurSideSweep] EnemyAttackExecutor not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ArthurSideSweep] AnimationComponent not found.");
        return;
    }

    m_arthurController->clearPath();
    m_arthurController->updateCurrentTarget();

	setupUI();

    Debug::log("[ArthurSideSweep] ENTER");
}

void ArthurSideSweep::OnStateUpdate()
{
    if (!m_arthurController || !m_attackConfig || !m_attackExecutor || !m_animation)
    {
        return;
    }

    if (m_arthurController->trySendDeathTrigger(m_animation))
    {
        return;
    }

    m_stateTimer += Time::getDeltaTime();

    updateUI();

    float hitTime = m_attackConfig->m_sideSweepHitTime;
    float totalDuration = m_attackConfig->m_sideSweepTotalDuration;

    if (m_arthurController->isPhase2())
    {
        hitTime = m_attackConfig->m_sideSweepPhase2HitTime;
        totalDuration = m_attackConfig->m_sideSweepPhase2TotalDuration;
    }

    if (!m_hasAppliedHit && m_stateTimer >= hitTime)
    {
        applyHit();
        m_hasAppliedHit = true;
    }

    if (m_stateTimer >= totalDuration)
    {
        goToRecover();
        return;
    }
}

void ArthurSideSweep::OnStateExit()
{
    if (m_attackConfig && m_attackConfig->m_sideSweepUICanvasTransform)
    {
		GameObjectAPI::setActive(m_attackConfig->m_sideSweepUICanvasTransform->getOwner(), false);
	}
    Debug::log("[ArthurSideSweep] EXIT");
}

void ArthurSideSweep::applyHit()
{
    if (!m_arthurController || !m_attackConfig || !m_attackExecutor)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 center = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 sweepDirection = m_arthurController->getSideSweepDirection(m_sweepSide);

    m_attackExecutor->applyDamageInCone(center, sweepDirection, m_attackConfig->m_sideSweepRange, m_attackConfig->m_sideSweepHalfAngleDegrees, m_attackConfig->m_sideSweepDamage, "SideSweep");

    Debug::log("[ArthurSideSweep] Hit applied. Side: %d", m_sweepSide);
}

void ArthurSideSweep::goToRecover()
{
    if (!m_attackConfig || !m_animation)
    {
        return;
    }

    if (m_arthurController)
    {
        float recoveryDuration = m_attackConfig->m_sideSweepRecoveryDuration;

        if (m_arthurController->isPhase2())
        {
            recoveryDuration = m_attackConfig->m_sideSweepPhase2RecoveryDuration;
        }

        m_arthurController->setRecoveryDuration(recoveryDuration);
    }

    Debug::log("[ArthurSideSweep] Going to Recover.");

    AnimationAPI::sendTrigger(m_animation, "ToRecover");
}

void ArthurSideSweep::setupUI()
{
    if (!m_attackConfig)
    {
        return;
    }

    Transform* canvasTransform = m_attackConfig->m_sideSweepUICanvasTransform;
	Transform2D* container = m_attackConfig->m_sideSweepUIContainerTransform2D;
    Transform2D* background = m_attackConfig->m_sideSweepUIBackgroundTransform2D;
    Transform2D* shadow = m_attackConfig->m_sideSweepUIShadowTransform2D;

    if (!canvasTransform || !container || !background || !shadow)
    {
        return;
    }

    GameObjectAPI::setActive(canvasTransform->getOwner(), true);
    if (m_sweepSide == -1)
    {
        TransformAPI::setRotationEuler(canvasTransform, Vector3(90.0f, 0.0f, -90.0f));
    }
    else
    {
        TransformAPI::setRotationEuler(canvasTransform, Vector3(90.0f, 0.0f, 90.0f));
	}
    Transform2DAPI::setAlpha(background, 0.0f);
    Transform2DAPI::setAlpha(shadow, 0.0f);
    Transform2DAPI::setAlpha(container, 1.0f);
}

void ArthurSideSweep::updateUI()
{
    if (!m_attackConfig)
    {
        return;
    }

    Transform* canvasTransform = m_attackConfig->m_sideSweepUICanvasTransform;
    Transform2D* container = m_attackConfig->m_sideSweepUIContainerTransform2D;
    Transform2D* background = m_attackConfig->m_sideSweepUIBackgroundTransform2D;
    Transform2D* shadow = m_attackConfig->m_sideSweepUIShadowTransform2D;

    if (!canvasTransform || !container || !background || !shadow)
    {
        return;
    }

    float hitTime = m_attackConfig->m_sideSweepHitTime;
    float totalDuration = m_attackConfig->m_sideSweepTotalDuration;
    if (m_arthurController->isPhase2())
    {
        hitTime = m_attackConfig->m_sideSweepPhase2HitTime;
        totalDuration = m_attackConfig->m_sideSweepPhase2TotalDuration;
	}

    if (m_stateTimer < hitTime)
    {
		const float t = std::clamp(m_stateTimer / hitTime, 0.0f, 1.0f);
        Transform2DAPI::setAlpha(background, t);
    }
    else
    {
        Transform2DAPI::setAlpha(background, 1.0f);
		const float t = std::clamp((m_stateTimer - hitTime) / (totalDuration - hitTime), 0.0f, 1.0f);
		const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, t);
        Transform2DAPI::setAlpha(shadow, easedT);
		Transform2DAPI::setAlpha(container, 1.0f - easedT);
	}
}

IMPLEMENT_SCRIPT(ArthurSideSweep)