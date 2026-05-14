#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyController;

class EnemySTUN : public StateMachineScript
{
	DECLARE_SCRIPT(EnemySTUN)

public:
	explicit EnemySTUN(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

	ScriptFieldList getExposedFields() const override;

private:
	EnemyController* m_enemyController = nullptr;
	float m_elapsedTime = 0.0f;

public:
	float m_stunDuration = 1.0f;
	bool m_debugEnabled = true;
};
