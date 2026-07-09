#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class ArthurAttackConfig;
class EnemyAttackExecutor;
class AnimationComponent;
class ArthurUI;
class ArthurSound;

class ArthurChargingSlam : public StateMachineScript
{
    DECLARE_SCRIPT(ArthurChargingSlam)

public:
    explicit ArthurChargingSlam(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

    FieldList getExposedFields() const override;

private:
    void lockTargetPosition();

    void startDash();
    void updateDash();

    void applyImpact();

    void tryApplyDashDamage(Transform* targetTransform, bool& hasDamagedTarget);

    void goToRecover();

    // Animations
    void setupAnimationPrepSection();
    void setupAnimationDashSection();
    void setupAnimationImpactSection();

    float getChargingDuration() const;
    float getDashSpeed() const;
    float getSafeSectionSpeed(float animationSectionDuration, float gameplayDuration) const;

private:
    ArthurBossController* m_arthurController = nullptr;
    ArthurAttackConfig* m_attackConfig = nullptr;
    EnemyAttackExecutor* m_attackExecutor = nullptr;
    AnimationComponent* m_animation = nullptr;
    ArthurUI* m_arthurUI = nullptr;
    ArthurSound* m_arthurSound = nullptr;

    float m_stateTimer = 0.0f;

    Vector3 m_startPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_lockedTargetPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_dashDirection = Vector3(0.0f, 0.0f, 0.0f);

    bool m_hasStartedDash = false;
    bool m_hasReachedDestination = false;
    bool m_hasAppliedImpact = false;

    bool m_hasDamagedFocusDuringDash = false;
    bool m_hasDamagedNonFocusDuringDash = false;

    // Animation Timings
    float m_animPrepStartTime = 0.0f;
    float m_animDashStartTime = 2.0f;
    float m_animImpactStartTime = 3.0f;
    float m_animEndTime = 4.0f;

    float m_previousAnimationSpeed = 1.0f;
};