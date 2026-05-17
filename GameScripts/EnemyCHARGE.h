#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyController;

class EnemyCHARGE : public StateMachineScript
{
	DECLARE_SCRIPT(EnemyCHARGE)

public:
	explicit EnemyCHARGE(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

	ScriptFieldList getExposedFields() const override;

private:
	EnemyController* m_enemyController = nullptr;
	Vector3 m_chargeDirection = Vector3(0.0f, 0.0f, 1.0f);
	float m_elapsedTime = 0.0f;

public:
	float m_chargeDuration = 0.5f;
	float m_chargeSpeed = 6.0f;
	float m_chargeCooldown = 3.0f;
	bool m_debugEnabled = true;
	
};
