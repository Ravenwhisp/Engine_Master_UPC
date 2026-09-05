#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class AelorinBossController;
class AnimationComponent;

class AelorinExhaustionState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinExhaustionState)

public:
	explicit AelorinExhaustionState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void finishExhaustion();

private:
	AelorinBossController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;

	float m_stateTimer = 0.0f;
	bool m_completed = false;
};