#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyController;

class EnemyATTACK : public StateMachineScript
{
	DECLARE_SCRIPT(EnemyATTACK)

public:
	explicit EnemyATTACK(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

	ScriptFieldList getExposedFields() const override;

private:
	void performAttack();

private:
	EnemyController* m_enemyController = nullptr;
	float m_attackTimer = 0.0f;

public:
	float m_attackDamage = 10.0f;
	float m_attackCooldown = 1.0f;
	float m_attackCommitDuration = 0.35f;
	float m_attackCommitTimer = 0.0f;
	bool m_debugEnabled = true;
	bool m_attackTimerInitialized = false;
	float m_attackTotalDuration = 0.75f;
	float m_damageTriggerTime = 0.25f;
	float m_stateTimer = 0.0f;
	bool m_hasAppliedDamage = false;
};
