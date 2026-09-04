#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class AelorinBossController;
class AnimationComponent;

class AelorinDeathState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinDeathState)

public:
	explicit AelorinDeathState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	AelorinBossController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;

	bool m_animationFinished = false;
};