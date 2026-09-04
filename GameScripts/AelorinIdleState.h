#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class AelorinBossController;
class AnimationComponent;

class AelorinIdleState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinIdleState)

public:
	explicit AelorinIdleState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	AelorinBossController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;

	float m_decisionTimer = 0.0f;
	bool m_decisionMade = false;
};