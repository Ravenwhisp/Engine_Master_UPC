#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SkeletonEnemyController;
class SkeletonDamageable;
class SkeletonAttackConfig;
class AnimationComponent;

class SkeletonReviveState : public StateMachineScript
{
	DECLARE_SCRIPT(SkeletonReviveState)

public:
	explicit SkeletonReviveState(GameObject* owner);
	FieldList getExposedFields() const override;

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	enum class Phase
	{
		ReviveStart,
		ReviveIdle,
		ReviveEnd
	};

	void changePhase(Phase phase);

	void updateReviveStart();
	void updateReviveIdle();
	void updateReviveEnd();

	void goToDeath();
	void goToChase();

private:
	SkeletonEnemyController* m_controller = nullptr;
	SkeletonDamageable* m_damageable = nullptr;
    AssetRef<SkeletonAttackConfig> m_attackConfig;
	AnimationComponent* m_animation = nullptr;

	Phase m_phase = Phase::ReviveStart;

	float m_phaseTimer = 0.0f;
	float m_reviveTimer = 0.0f;
};