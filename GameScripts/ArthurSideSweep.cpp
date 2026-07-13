#include "pch.h"
#include "ArthurSideSweep.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "EnemyAttackExecutor.h"
#include "ArthurUI.h"
#include "ArthurSound.h"

IMPLEMENT_SCRIPT_FIELDS(ArthurSideSweep,
    SERIALIZED_ASSET_REF(m_attackConfig, "Arthur Attack Config", AssetType::DATA_CONTAINER),
    SERIALIZED_INT(m_sweepSide, "Sweep Side")
)

ArthurSideSweep::ArthurSideSweep(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurSideSweep::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());
    m_arthurUI = GameObjectAPI::findScript<ArthurUI>(getOwner());
    m_arthurSound = GameObjectAPI::findScript<ArthurSound>(getOwner());

    m_stateTimer = 0.0f;
    m_hasAppliedHit = false;

    if (!m_arthurController)
    {
        Debug::error("[ArthurSideSweep] ArthurBossController not found.");
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

    if (!m_arthurUI)
    {
        Debug::error("[ArthurSideSweep] ArthurUI not found.");
        return;
    }

    m_arthurController->clearPath();
    m_arthurController->resetRepathTimer();

    m_arthurUI->setupSideSweepUI(m_sweepSide);

    if (m_arthurSound)
    {
        m_arthurSound->playSideSweep();
    }

    Debug::log("[ArthurSideSweep] ENTER");
}

void ArthurSideSweep::OnStateUpdate()
{
    if (!m_arthurController || !m_attackExecutor || !m_animation)
    {
        return;
    }

    const ArthurAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    if (m_arthurController->trySendDeathTrigger(m_animation))
    {
        return;
    }

    m_stateTimer += Time::getDeltaTime();

    float hitTime = cfg->m_sideSweepHitTime;
    float totalDuration = cfg->m_sideSweepTotalDuration;

    if (m_arthurController->isPhase2())
    {
        hitTime = cfg->m_sideSweepPhase2HitTime;
        totalDuration = cfg->m_sideSweepPhase2TotalDuration;
    }

    if (m_arthurUI)
    {
        m_arthurUI->updateSideSweepUI(m_stateTimer, hitTime, totalDuration);
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
    if (m_arthurUI)
    {
        m_arthurUI->hideSideSweepUI();
    }

    Debug::log("[ArthurSideSweep] EXIT");
}

void ArthurSideSweep::applyHit()
{
    if (!m_arthurController || !m_attackExecutor)
    {
        return;
    }

    const ArthurAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
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

    m_attackExecutor->applyDamageInCone(center, sweepDirection, cfg->m_sideSweepRange, cfg->m_sideSweepHalfAngleDegrees, cfg->m_sideSweepDamage, "SideSweep");

    if (m_arthurSound)
    {
        m_arthurSound->playSideImpact();
    }

    Debug::log("[ArthurSideSweep] Hit applied. Side: %d", m_sweepSide);
}

void ArthurSideSweep::goToRecover()
{
    if (!m_animation)
    {
        return;
    }

    const ArthurAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    if (m_arthurController)
    {
        float recoveryDuration = cfg->m_sideSweepRecoveryDuration;

        if (m_arthurController->isPhase2())
        {
            recoveryDuration = cfg->m_sideSweepPhase2RecoveryDuration;
        }

        m_arthurController->setRecoveryDuration(recoveryDuration);
    }

    Debug::log("[ArthurSideSweep] Going to Recover.");

    AnimationAPI::sendTrigger(m_animation, "ToRecover");
}

IMPLEMENT_SCRIPT(ArthurSideSweep)