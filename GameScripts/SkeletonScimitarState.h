#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SkeletonEnemyController;
class EnemyAttackExecutor;
class AnimationComponent;
class SkeletonParticles;
class SkeletonUI;

class SkeletonScimitarState : public StateMachineScript
{
	DECLARE_SCRIPT(SkeletonScimitarState)

public:
	explicit SkeletonScimitarState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	enum class Phase
	{
		Dash,
		Attack1,
		Reaim1,
		Attack2,
		Backstep,
		Reaim2,
		Attack3,
		Finished
	};

	void changePhase(Phase phase);

	void updateDash();
	void updateAttack();
	void updateBackstep();

	void setupAttackTelegraph();
	void applyHit(bool shouldStun);
	void moveInDirection(const Vector3& direction, float speed);
	void goToChase();

	float getScimitarAttackClipDuration() const;
	float getScimitarAttackHitTime() const;

private:
	SkeletonEnemyController* m_controller = nullptr;
	EnemyAttackExecutor* m_attackExecutor = nullptr;
	AnimationComponent* m_animation = nullptr;
	SkeletonParticles* m_particles = nullptr;
	SkeletonUI* m_skeletonUI = nullptr;

	Phase m_phase = Phase::Dash;
	float m_phaseTimer = 0.0f;
	bool m_hasAppliedHit = false;
	float m_previousAnimationSpeed = 1.0f;
	float m_reaimDuration = 0.2f;
};