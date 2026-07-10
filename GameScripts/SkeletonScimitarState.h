#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SkeletonEnemyController;
class SkeletonAttackConfig;
class EnemyAttackExecutor;
class AnimationComponent;

class SkeletonScimitarState : public StateMachineScript
{
	DECLARE_SCRIPT(SkeletonScimitarState)

public:
	explicit SkeletonScimitarState(GameObject* owner);
	FieldList getExposedFields() const override;

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	enum class Phase
	{
		Dash,
		Attack1,
		Attack2,
		Backstep,
		Attack3,
		Finished
	};

	void changePhase(Phase phase);

	void updateDash();
	void updateAttack();
	void updateBackstep();

	void applyHit(bool shouldStun);
	void moveInDirection(const Vector3& direction, float speed);
	void goToChase();

	float getScimitarAttackClipDuration() const;
	float getScimitarAttackHitTime() const;

private:
	SkeletonEnemyController* m_controller = nullptr;
    AssetRef<SkeletonAttackConfig> m_attackConfig;
	EnemyAttackExecutor* m_attackExecutor = nullptr;
	AnimationComponent* m_animation = nullptr;

	Phase m_phase = Phase::Dash;
	float m_phaseTimer = 0.0f;
	bool m_hasAppliedHit = false;
	float m_previousAnimationSpeed = 1.0f;
};