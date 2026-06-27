#include "pch.h"
#include "ArthurChargingSlam.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "EnemyAttackExecutor.h"
#include "ArthurUI.h"

#include "Transform2D.h"

IMPLEMENT_SCRIPT_FIELDS(ArthurChargingSlam,
    SERIALIZED_FLOAT(m_animPrepStartTime, "Anim Prep Start Time", 0.0f, 20.0f, 0.01f),
    SERIALIZED_FLOAT(m_animDashStartTime, "Anim Dash Start Time", 0.0f, 20.0f, 0.01f),
    SERIALIZED_FLOAT(m_animImpactStartTime, "Anim Impact Start Time", 0.0f, 20.0f, 0.01f),
    SERIALIZED_FLOAT(m_animEndTime, "Anim End Time", 0.0f, 20.0f, 0.01f)
)

ArthurChargingSlam::ArthurChargingSlam(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurChargingSlam::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());
    m_arthurUI = GameObjectAPI::findScript<ArthurUI>(getOwner());

    m_stateTimer = 0.0f;

    m_startPosition = Vector3(0.0f, 0.0f, 0.0f);
    m_lockedTargetPosition = Vector3(0.0f, 0.0f, 0.0f);
    m_dashDirection = Vector3(0.0f, 0.0f, 0.0f);

    m_hasStartedDash = false;
    m_hasReachedDestination = false;
    m_hasAppliedImpact = false;

    m_hasDamagedFocusDuringDash = false;
    m_hasDamagedNonFocusDuringDash = false;

    if (!m_arthurController)
    {
        Debug::error("[ArthurChargingSlam] ArthurBossController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[ArthurChargingSlam] ArthurAttackConfig not found.");
        return;
    }

    if (!m_attackExecutor)
    {
        Debug::error("[ArthurChargingSlam] EnemyAttackExecutor not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ArthurChargingSlam] AnimationComponent not found.");
        return;
    }

    if (!m_arthurUI)
    {
        Debug::error("[ArthurChargingSlam] ArthurUI not found.");
        return;
    }

    m_arthurController->clearPath();
    m_arthurController->resetRepathTimer();

    m_arthurController->updateCurrentTarget();
    m_arthurController->faceCurrentTarget();

    m_previousAnimationSpeed = AnimationAPI::getSpeedMultiplier(m_animation);

    lockTargetPosition();

    setupAnimationPrepSection();

    m_arthurUI->setupChargingSlamUI(m_startPosition, m_lockedTargetPosition, m_dashDirection);

    Debug::log("[ArthurChargingSlam] ENTER");
}

void ArthurChargingSlam::OnStateUpdate()
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

    float chargingDuration = m_attackConfig->m_chargingSlamHitTime;

    if (m_arthurController->isPhase2())
    {
        chargingDuration = m_attackConfig->m_chargingSlamPhase2HitTime;
    }

    if (!m_hasStartedDash)
    {
        m_arthurController->facePosition(m_lockedTargetPosition);
    }

    if (!m_hasStartedDash && m_stateTimer >= chargingDuration)
    {
        startDash();
    }

    if (m_hasStartedDash && !m_hasReachedDestination)
    {
        updateDash();
    }

    if (m_hasReachedDestination && !m_hasAppliedImpact)
    {
        applyImpact();
        m_hasAppliedImpact = true;
    }

    if (m_hasAppliedImpact && m_stateTimer >= m_attackConfig->m_chargingSlamTotalDuration)
    {
        goToRecover();
        return;
    }

    if (m_arthurUI)
    {
        m_arthurUI->updateChargingSlamUI(m_stateTimer, m_arthurController->isPhase2(), m_hasStartedDash, m_hasAppliedImpact, m_startPosition, m_lockedTargetPosition, chargingDuration);
    }
}

void ArthurChargingSlam::OnStateExit()
{
    if (m_animation)
    {
        AnimationAPI::setSpeedMultiplier(m_animation, m_previousAnimationSpeed);
    }

    if (m_arthurUI)
    {
        m_arthurUI->hideChargingSlamUI();
    }

    Debug::log("[ArthurChargingSlam] EXIT");
}

void ArthurChargingSlam::lockTargetPosition()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    m_startPosition = TransformAPI::getGlobalPosition(ownerTransform);

    Transform* focusTarget = m_arthurController->getFocusTarget();
    if (!focusTarget)
    {
        m_lockedTargetPosition = m_startPosition;
        m_dashDirection = Vector3(0.0f, 0.0f, 0.0f);
        return;
    }

    m_lockedTargetPosition = TransformAPI::getGlobalPosition(focusTarget);
    m_lockedTargetPosition.y = m_startPosition.y;

    m_dashDirection = m_lockedTargetPosition - m_startPosition;
    m_dashDirection.y = 0.0f;

    if (m_dashDirection.LengthSquared() < 0.0001f)
    {
        m_dashDirection = Vector3(0.0f, 0.0f, 0.0f);
        return;
    }

    m_dashDirection.Normalize();
}

void ArthurChargingSlam::startDash()
{
    m_hasStartedDash = true;
    setupAnimationDashSection();

    if (m_dashDirection.LengthSquared() < 0.0001f)
    {
        m_hasReachedDestination = true;
        return;
    }

    Debug::log("[ArthurChargingSlam] Dash started.");
}

void ArthurChargingSlam::updateDash()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 currentPosition = TransformAPI::getGlobalPosition(ownerTransform);

    Vector3 toDestination = m_lockedTargetPosition - currentPosition;
    toDestination.y = 0.0f;

    const float remainingDistance = toDestination.Length();

    float dashSpeed = m_attackConfig->m_chargingSlamDashSpeed;

    if (m_arthurController->isPhase2())
    {
        dashSpeed = m_attackConfig->m_chargingSlamPhase2DashSpeed;
    }

    const float stepDistance = dashSpeed * Time::getDeltaTime();

    if (remainingDistance <= 0.05f)
    {
        TransformAPI::setGlobalPosition(ownerTransform, m_lockedTargetPosition);
        m_hasReachedDestination = true;
        return;
    }

    if (stepDistance >= remainingDistance)
    {
        TransformAPI::setGlobalPosition(ownerTransform, m_lockedTargetPosition);
        m_hasReachedDestination = true;
        return;
    }

    currentPosition += m_dashDirection * stepDistance;
    TransformAPI::setGlobalPosition(ownerTransform, currentPosition);

    Transform* focusTarget = m_arthurController->getFocusTarget();
    Transform* nonFocusTarget = m_arthurController->getNonFocusTarget();

    tryApplyDashDamage(focusTarget, m_hasDamagedFocusDuringDash);
    tryApplyDashDamage(nonFocusTarget, m_hasDamagedNonFocusDuringDash);
}

void ArthurChargingSlam::tryApplyDashDamage(Transform* targetTransform, bool& hasDamagedTarget)
{
    if (hasDamagedTarget)
    {
        return;
    }

    if (!m_attackExecutor || !m_attackConfig)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 center = TransformAPI::getGlobalPosition(ownerTransform);

    const bool damaged = m_attackExecutor->tryDamageTargetInRadius(targetTransform, center, m_attackConfig->m_chargingSlamDashHitRadius, m_attackConfig->m_chargingSlamDashDamage, "ChargingSlamDash");

    if (damaged)
    {
        hasDamagedTarget = true;
    }
}

void ArthurChargingSlam::applyImpact()
{
    if (!m_attackExecutor || !m_attackConfig)
    {
        return;
    }

    setupAnimationImpactSection();

    if (m_arthurUI)
    {
        m_arthurUI->startChargingSlamImpactUI();
    }

    m_attackExecutor->applyDamageAndStunInRadius(m_lockedTargetPosition, m_attackConfig->m_chargingSlamImpactRadius, m_attackConfig->m_chargingSlamFinalAreaImpactDamage, m_attackConfig->m_chargingSlamImpactStunDuration, "ChargingSlamImpact");

    Debug::log("[ArthurChargingSlam] Impact applied.");
}

void ArthurChargingSlam::goToRecover()
{
    if (!m_attackConfig || !m_animation)
    {
        return;
    }

    if (m_arthurController)
    {
        m_arthurController->setRecoveryDuration(m_attackConfig->m_chargingSlamRecoveryDuration);
    }

    Debug::log("[ArthurChargingSlam] Going to Recover.");

    AnimationAPI::sendTrigger(m_animation, "ToRecover");
}

float ArthurChargingSlam::getChargingDuration() const
{
    if (!m_attackConfig)
    {
        return 0.0f;
    }

    float chargingDuration = m_attackConfig->m_chargingSlamHitTime;

    if (m_arthurController && m_arthurController->isPhase2())
    {
        chargingDuration = m_attackConfig->m_chargingSlamPhase2HitTime;
    }

    return chargingDuration;
}

float ArthurChargingSlam::getDashSpeed() const
{
    if (!m_attackConfig)
    {
        return 0.0f;
    }

    float dashSpeed = m_attackConfig->m_chargingSlamDashSpeed;

    if (m_arthurController && m_arthurController->isPhase2())
    {
        dashSpeed = m_attackConfig->m_chargingSlamPhase2DashSpeed;
    }

    return dashSpeed;
}

float ArthurChargingSlam::getSafeSectionSpeed(float animationSectionDuration, float gameplayDuration) const
{
    if (animationSectionDuration <= 0.001f)
    {
        return 1.0f;
    }

    if (gameplayDuration <= 0.001f)
    {
        return 1.0f;
    }

    return animationSectionDuration / gameplayDuration;
}

void ArthurChargingSlam::setupAnimationPrepSection()
{
    if (!m_animation)
    {
        return;
    }

    const float animationPrepDuration = m_animDashStartTime - m_animPrepStartTime;
    const float gameplayPrepDuration = getChargingDuration();

    const float speed = getSafeSectionSpeed(animationPrepDuration, gameplayPrepDuration);

    AnimationAPI::setPlaybackTime(m_animation, m_animPrepStartTime);
    AnimationAPI::setSpeedMultiplier(m_animation, speed);
}

void ArthurChargingSlam::setupAnimationDashSection()
{
    if (!m_animation)
    {
        return;
    }

    const float animationDashDuration = m_animImpactStartTime - m_animDashStartTime;

    const float distance = Vector3::Distance(m_startPosition, m_lockedTargetPosition);
    const float dashSpeed = getDashSpeed();

    float gameplayDashDuration = 0.0f;

    if (dashSpeed > 0.001f)
    {
        gameplayDashDuration = distance / dashSpeed;
    }

    const float speed = getSafeSectionSpeed(animationDashDuration, gameplayDashDuration);

    AnimationAPI::setPlaybackTime(m_animation, m_animDashStartTime);
    AnimationAPI::setSpeedMultiplier(m_animation, speed);
}

void ArthurChargingSlam::setupAnimationImpactSection()
{
    if (!m_animation || !m_attackConfig)
    {
        return;
    }

    const float animationImpactDuration = m_animEndTime - m_animImpactStartTime;
    const float gameplayImpactDuration = m_attackConfig->m_chargingSlamTotalDuration - m_stateTimer;

    const float speed = getSafeSectionSpeed(animationImpactDuration, gameplayImpactDuration);

    AnimationAPI::setPlaybackTime(m_animation, m_animImpactStartTime);
    AnimationAPI::setSpeedMultiplier(m_animation, speed);
}

IMPLEMENT_SCRIPT(ArthurChargingSlam)