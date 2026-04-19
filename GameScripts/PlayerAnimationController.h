#pragma once

#include "ScriptAPI.h"

class AnimationComponent;

enum class AnimState
{
    Idle,
    Move,
    Dash,
    Attack,
    Damaged,
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

    ScriptFieldList getExposedFields() const override;

    void setMoving(bool moving);
    void setDashing(bool dashing);
    void setDowned(bool downed);
    void setDead(bool dead);

    void requestAttack();
    void requestDamaged();

private:
    AnimationComponent* findAnimationComponent();
    bool playAnimState(AnimState state, float blendTime);

public:
    std::string m_idleStateName = "";
	std::string m_moveStateName = "";
	std::string m_dashStateName = "";
	std::string m_attackStateName = "";
	std::string m_damagedStateName = "";
    std::string m_downedStateName = "";
	std::string m_deathStateName = "";

    float m_defaultBlendTime = 0.25f;
    float m_attackBlendTime = 0.15f;
    float m_damagedBlendTime = 0.10f;
    float m_downedBlendTime = 0.10f;
    float m_deathBlendTime = 0.10f;

private:
    AnimationComponent* m_animationComponent = nullptr;

    bool m_isMoving = false;
    bool m_isDashing = false;
    bool m_isDowned = false;
    bool m_isDead = false;

    bool m_attackRequested = false;
    bool m_damagedRequested = false;

    AnimState m_currentState = AnimState::Idle;
};