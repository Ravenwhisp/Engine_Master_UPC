#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class AnimationComponent;


class ArthurIdle : public StateMachineScript
{
	DECLARE_SCRIPT(ArthurIdle)

public:
	explicit ArthurIdle(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	ArthurBossController* m_arthurController = nullptr;
	AnimationComponent* m_animation = nullptr;
};