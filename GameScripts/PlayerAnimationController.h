#pragma once

#include "ScriptAPI.h"

#include <string>
#include <vector>

class AnimationComponent;

enum class AnimState
{
    Idle,
    Move,
    Dash,
    Attack,
    ChargeHold,
    Damaged,
    Recovery,
    Downed,
    Death
};

class PlayerAnimationController : public Script
{
    DECLARE_SCRIPT(PlayerAnimationController)

public:
    explicit PlayerAnimationController(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

    void setMoving(bool moving);
    void setDashing(bool dashing, float dashDurationSeconds = 0.0f);
    void setDowned(bool downed);
    void setDead(bool dead);

    void requestAttack();
    void requestDamaged();

    void setAttackOverride(const std::string& stateName, float blendTime, float speed);
    void clearAttackOverride();

    void playRecovery(const std::string& stateName, float blendTime, float speed);

    void beginChargeHold(const std::string& stateName, float blendTime, float speed, float pausePct);
    void setChargeProgress(float progress01);
    void endChargeHold(float releaseFraction = 1.0f);

private:
    AnimationComponent* findAnimationComponent();
    bool playAnimState(AnimState state, float blendTime);
    const std::string& pickDamagedState();

public:
    std::string m_idleStateName = "";
	std::string m_moveStateName = "";
	std::string m_dashStateName = "";
	std::string m_attackStateName = "";
	std::vector<std::string> m_damagedStateNames;
    std::string m_downedStateName = "";
	std::string m_deathStateName = "";

    float m_defaultBlendTime = 0.25f;
    float m_attackBlendTime = 0.15f;
    float m_dashBlendTime = 0.06f;
    float m_damagedBlendTime = 0.10f;
    float m_downedBlendTime = 0.10f;
    float m_deathBlendTime = 0.10f;

    float m_dashAnimDuration = 0.5f;
    float m_dashAnimStartPct = 0.2f;

private:
    AnimationComponent* m_animationComponent = nullptr;

    bool m_isMoving = false;
    bool m_isDashing = false;
    bool m_isDowned = false;
    bool m_isDead = false;

    bool m_attackRequested = false;
    bool m_damagedRequested = false;

    AnimState m_currentState = AnimState::Idle;

    std::string m_attackOverrideName = "";
    float m_attackOverrideBlend = 0.15f;
    float m_attackOverrideSpeed = 1.0f;
    bool  m_hasAttackOverride = false;

    int m_damagedIndex = -1;
    std::string m_currentDamagedState = "";
    float m_damagedHoldTimer = 0.0f;

    std::string m_recoveryState = "";
    float m_recoveryBlend = 0.15f;
    float m_recoverySpeed = 1.0f;
    float m_recoveryHoldTimer = 0.0f;

    bool m_chargeHoldActive = false;
    bool m_chargeHoldPaused = false;
    std::string m_chargeHoldState = "";
    float m_chargeHoldBlend = 0.15f;
    float m_chargeHoldSpeed = 1.0f;
    float m_chargeHoldPausePct = 0.5f;
    float m_chargeProgress = 0.0f;
    float m_chargeReleaseTimer = 0.0f;

    float m_dashMoveDuration = 0.4f;
    float m_dashHoldTimer = 0.0f;
    bool  m_dashJustStarted = false;
};