#include "pch.h"
#include "ArthurHeavySwipe.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "EnemyAttackExecutor.h"

ArthurHeavySwipe::ArthurHeavySwipe(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurHeavySwipe::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    m_stateTimer = 0.0f;

    m_hit1Applied = false;
    m_hit2Applied = false;
    m_hit3Applied = false;
    m_hit4Applied = false;

    if (!m_arthurController)
    {
        Debug::error("[ArthurHeavySwipe] ArthurBossController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[ArthurHeavySwipe] ArthurAttackConfig not found.");
        return;
    }

    if (!m_attackExecutor)
    {
        Debug::error("[ArthurHeavySwipe] EnemyAttackExecutor not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ArthurHeavySwipe] AnimationComponent not found.");
        return;
    }

    m_previousAnimationSpeed = AnimationAPI::getSpeedMultiplier(m_animation);

    float animationSpeed = m_phase1AnimationSpeed;

    if (m_arthurController->isPhase2())
    {
        animationSpeed = m_phase2AnimationSpeed;
    }

    AnimationAPI::setSpeedMultiplier(m_animation, animationSpeed);

    m_arthurController->clearPath();
    m_arthurController->updateCurrentTarget();
    m_arthurController->faceCurrentTarget();

    setupUI();

    Debug::log("[ArthurHeavySwipe] ENTER");
}

void ArthurHeavySwipe::OnStateUpdate()
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

    const bool isPhase2 = m_arthurController->isPhase2();

    float hit1Time = m_attackConfig->m_heavySwipeHit1Time;
    float hit2Time = m_attackConfig->m_heavySwipeHit2Time;
    float hit3Time = m_attackConfig->m_heavySwipeHit3Time;

    if (isPhase2)
    {
        hit1Time = m_attackConfig->m_heavySwipePhase2Hit1Time;
        hit2Time = m_attackConfig->m_heavySwipePhase2Hit2Time;
        hit3Time = m_attackConfig->m_heavySwipePhase2Hit3Time;
    }

    if (!m_hit1Applied && m_stateTimer >= hit1Time)
    {
        tryApplyHit(1);
        m_hit1Applied = true;
    }

    if (!m_hit2Applied && m_stateTimer >= hit2Time)
    {
        tryApplyHit(2);
        m_hit2Applied = true;
    }

    if (!m_hit3Applied && m_stateTimer >= hit3Time)
    {
        tryApplyHit(3);
        m_hit3Applied = true;
    }

    if (isPhase2 && !m_hit4Applied && m_stateTimer >= m_attackConfig->m_heavySwipePhase2Hit4Time)
    {
        tryApplyHit(4);
        m_hit4Applied = true;
    }

    if (m_stateTimer >= m_attackConfig->m_heavySwipeTotalDuration)
    {
        goToRecover();
        return;
    }
}

void ArthurHeavySwipe::OnStateExit()
{
    AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
    if (animation)
    {
        AnimationAPI::setSpeedMultiplier(animation, m_previousAnimationSpeed);
    }

	if (m_attackConfig && m_attackConfig->m_heavySwipeUICanvasTransform)
    {
        GameObjectAPI::setActive(m_attackConfig->m_heavySwipeUICanvasTransform->getOwner(), false);
    }

    Debug::log("[ArthurHeavySwipe] EXIT");
}

void ArthurHeavySwipe::tryApplyHit(int hitIndex)
{
    if (!m_attackConfig || !m_attackExecutor)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 center = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 forward = TransformAPI::getForward(ownerTransform);

    m_attackExecutor->applyDamageInCone(center, forward, m_attackConfig->m_heavySwipeRange, m_attackConfig->m_heavySwipeHalfAngleDegrees, m_attackConfig->m_heavySwipeDamage, "HeavySwipe");

    Debug::log("[ArthurHeavySwipe] Hit %d applied.", hitIndex);
}

void ArthurHeavySwipe::goToRecover()
{
    if (!m_attackConfig || !m_animation)
    {
        return;
    }

    if (m_arthurController)
    {
        float recoveryDuration = m_attackConfig->m_heavySwipeRecoveryDuration;

        if (m_arthurController->isPhase2())
        {
            recoveryDuration = m_attackConfig->m_heavySwipePhase2RecoveryDuration;
        }

        m_arthurController->setRecoveryDuration(recoveryDuration);
    }

    Debug::log("[ArthurHeavySwipe] Going to Recover.");

    AnimationAPI::sendTrigger(m_animation, "ToRecover");
}

void ArthurHeavySwipe::setupUI()
{
    if (!m_attackConfig)
    {
        return;
    }

    Transform* canvas = m_attackConfig->m_heavySwipeUICanvasTransform;
    Transform2D* container = m_attackConfig->m_heavySwipeUIContainerTransform2D;
    Transform2D* background = m_attackConfig->m_heavySwipeUIBackgroundTransform2D;
    Transform2D* border = m_attackConfig->m_heavySwipeUIBorderTransform2D;
    Transform2D* glow = m_attackConfig->m_heavySwipeUIGlowTransform2D;
    Transform2D* rightClaw = m_attackConfig->m_heavySwipeUIRightClawTransform2D;
    Transform2D* leftClaw = m_attackConfig->m_heavySwipeUILeftClawTransform2D;

    if (!canvas || !container || !background || !border || !glow || !rightClaw || !leftClaw)
    {
        return;
    }

    GameObjectAPI::setActive(canvas->getOwner(), true);

    Transform2DAPI::setAlpha(container, 0.0f);
    Transform2DAPI::setAlpha(background, 1.0f);
    Transform2DAPI::setAlpha(border, 1.0f);
    Transform2DAPI::setAlpha(glow, 0.0f);
    Transform2DAPI::setAlpha(rightClaw, 0.0f);
    Transform2DAPI::setAlpha(leftClaw, 0.0f);
}

void ArthurHeavySwipe::updateUI()
{
    if (!m_attackConfig)
    {
        return;
    }

	Transform2D* container = m_attackConfig->m_heavySwipeUIContainerTransform2D;
	Transform2D* background = m_attackConfig->m_heavySwipeUIBackgroundTransform2D;
	Transform2D* border = m_attackConfig->m_heavySwipeUIBorderTransform2D;
	Transform2D* glow = m_attackConfig->m_heavySwipeUIGlowTransform2D;
	Transform2D* rightClaw = m_attackConfig->m_heavySwipeUIRightClawTransform2D;
	Transform2D* leftClaw = m_attackConfig->m_heavySwipeUILeftClawTransform2D;

    if (!container || !background || !border || !glow || !rightClaw || !leftClaw)
    {
        return;
    }

    const bool isPhase2 = m_arthurController->isPhase2();
    float hit1Time = m_attackConfig->m_heavySwipeHit1Time;
    float hit2Time = m_attackConfig->m_heavySwipeHit2Time;
    float hit3Time = m_attackConfig->m_heavySwipeHit3Time;

    if (isPhase2)
    {
        hit1Time = m_attackConfig->m_heavySwipePhase2Hit1Time;
        hit2Time = m_attackConfig->m_heavySwipePhase2Hit2Time;
        hit3Time = m_attackConfig->m_heavySwipePhase2Hit3Time;
    }


    if (m_stateTimer < hit1Time)
    {
        // Charging
		const float t = std::clamp(m_stateTimer / hit1Time, 0.0f, 1.0f);
		Transform2DAPI::setAlpha(container, t);
    }
    else if (m_stateTimer < hit2Time)
    {
        // Attack 1
        const float t = (m_stateTimer - hit1Time) / (hit2Time - hit1Time);
        applyHitEffects(t, glow, border, leftClaw);
    }
    else if (m_stateTimer < hit3Time)
    {
        // Attack 2
        const float t = (m_stateTimer - hit2Time) / (hit3Time - hit2Time);
        applyHitEffects(t, glow, border, rightClaw);
    }
    else if (isPhase2 && m_stateTimer < m_attackConfig->m_heavySwipePhase2Hit4Time)
    {
        // Attack 3
        const float t = (m_stateTimer - hit3Time) / (m_attackConfig->m_heavySwipePhase2Hit4Time - hit3Time);
		applyHitEffects(t, glow, border, leftClaw);
    }
    else if (m_stateTimer <= m_attackConfig->m_heavySwipeTotalDuration)
    {
		const float lastHitTime = isPhase2 ? m_attackConfig->m_heavySwipePhase2Hit4Time : hit3Time;
        const float t = (m_stateTimer - lastHitTime) / (m_attackConfig->m_heavySwipeTotalDuration - lastHitTime);
        applyHitEffects(t, glow, border, isPhase2 ? rightClaw : leftClaw);
		const float alpha = MathAPI::moveTowards(t, 1.0f, 0.3f);
        Transform2DAPI::setAlpha(container, alpha);
        return;
    }

}
void ArthurHeavySwipe::applyHitEffects(float t, Transform2D* glow, Transform2D* border, Transform2D* claw)
{
    const float glowAlpha = MathAPI::pingPong(t);
    Transform2DAPI::setAlpha(glow, glowAlpha);

    const float borderScale =
        MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, std::clamp(t, 0.1f, 1.0f))
        * m_attackConfig->m_heavySwipeRange;

    Transform2DAPI::setScale(border, Vector2(borderScale, borderScale));

    const float anchorValue = MathAPI::lerp(0.5f, 1.0f, t);

	Transform2DAPI::setAlpha(claw, t);
    Transform2DAPI::setAnchorMin(claw, Vector2(0.5f, anchorValue));
}

IMPLEMENT_SCRIPT(ArthurHeavySwipe)