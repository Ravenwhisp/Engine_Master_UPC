#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyController;

class EnemyRECOVER : public StateMachineScript
{
	DECLARE_SCRIPT(EnemyRECOVER)

public:
	explicit EnemyRECOVER(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

	ScriptFieldList getExposedFields() const override;

private:
	EnemyController* m_enemyController = nullptr;
	float m_recoverTimer = 0.0f;

public:
	float m_recoverDuration = 0.6f;
	bool m_debugEnabled = true;
	bool m_goToStunAfterRecover = false;
};
