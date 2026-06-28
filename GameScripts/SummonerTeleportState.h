#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SummonerEnemyController;
class AnimationComponent;

class SummonerTeleportState : public StateMachineScript
{
	DECLARE_SCRIPT(SummonerTeleportState)

public:
	explicit SummonerTeleportState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	SummonerEnemyController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;
};