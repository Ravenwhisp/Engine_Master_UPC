#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyDeathState : public StateMachineScript
{
	DECLARE_SCRIPT(EnemyDeathState)

public:
	explicit EnemyDeathState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

	ScriptFieldList getExposedFields() const override;

	void pauseDeathCountdown();
	void resumeDeathCountdown();
	void finalizeDeathNow();
	void abortDeathForRevival();
	bool isDeathActive() const { return m_waitingToDestroy && !m_deathFinished; }

protected:
	virtual void onDeathStarted(); // use this to trigger specific logic like open a locked door etc
	virtual void onDeathFinished();
	virtual void dropRewards();

	void destroyEnemyNow();
	void startDestroyCountdown(float delay);

protected:
	float m_destroyDelay = 2.0f;
	float m_deathTimer = 0.0f;
	
	AssetRef<Prefab> m_healthPrefab;
	bool m_shouldDropHealth = true;
	int m_healthDropQuantity = 1;
	float m_dropHealAmount = 10.0f;
	float m_dropRadius = 2.0f;
	float m_dropHeight = 1.0f;  // Y offset from floor to enemy center (arc spawn point)

private:
	bool m_waitingToDestroy = false;
	bool m_deathFinished = false;
	bool m_deathPaused = false;
};