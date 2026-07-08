#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class MeleeEnemyController;
class AnimationComponent;
class PaladinVFX;

class PaladinChaseState : public StateMachineScript
{
	DECLARE_SCRIPT(PaladinChaseState)

public:
	explicit PaladinChaseState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void stopWalkingDust();

	MeleeEnemyController* m_paladinController = nullptr;
	AnimationComponent* m_animation = nullptr;
	PaladinVFX* m_paladinVFX = nullptr;
};