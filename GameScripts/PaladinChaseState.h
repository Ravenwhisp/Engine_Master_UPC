#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class MeleeEnemyController;
class AnimationComponent;

class PaladinChaseState : public StateMachineScript
{
	DECLARE_SCRIPT(PaladinChaseState)

public:
	explicit PaladinChaseState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	MeleeEnemyController* m_paladinController = nullptr;
	AnimationComponent* m_animation = nullptr;
};