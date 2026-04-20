#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyController;

class EnemyIDLE : public StateMachineScript
{
	DECLARE_SCRIPT(EnemyIDLE)

public:
	explicit EnemyIDLE(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

	ScriptFieldList getExposedFields() const override;

private:
	EnemyController* m_enemyController = nullptr;

public:
	bool m_debugEnabled = true;
};