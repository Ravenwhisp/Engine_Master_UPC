#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SummonerEnemyController;
class SummonerAttackConfig;
class AnimationComponent;

class SummonerSummonState : public StateMachineScript
{
	DECLARE_SCRIPT(SummonerSummonState)

public:
	explicit SummonerSummonState(GameObject* owner);
	FieldList getExposedFields() const override;

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	SummonerEnemyController* m_controller = nullptr;
    AssetRef<SummonerAttackConfig> m_attackConfig;
	AnimationComponent* m_animation = nullptr;

	float m_stateTimer = 0.0f;
	bool m_hasSummoned = false;
};