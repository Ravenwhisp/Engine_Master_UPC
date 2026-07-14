#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SummonerEnemyController;
class AnimationComponent;

class SummonerIdleState : public StateMachineScript
{
	DECLARE_SCRIPT(SummonerIdleState)

public:
	explicit SummonerIdleState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	SummonerEnemyController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;
};