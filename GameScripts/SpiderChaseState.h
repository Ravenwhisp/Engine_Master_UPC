#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SpiderEnemyController;
class AnimationComponent;

class SpiderChaseState : public StateMachineScript
{
	DECLARE_SCRIPT(SpiderChaseState)

public:
	explicit SpiderChaseState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	SpiderEnemyController* m_spiderController = nullptr;
	AnimationComponent* m_animation = nullptr;
};