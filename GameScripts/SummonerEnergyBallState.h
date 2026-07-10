#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SummonerEnemyController;
class SummonerAttackConfig;
class AnimationComponent;

class SummonerEnergyBallState : public StateMachineScript
{
	DECLARE_SCRIPT(SummonerEnergyBallState)

public:
	explicit SummonerEnergyBallState(GameObject* owner);
	FieldList getExposedFields() const override;

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void spawnEnergyBall();

private:
	SummonerEnemyController* m_controller = nullptr;
    AssetRef<SummonerAttackConfig> m_attackConfig;
	AnimationComponent* m_animation = nullptr;

	Transform* m_committedTarget = nullptr;

	float m_stateTimer = 0.0f;
	bool m_hasFiredEnergyBall = false;
};