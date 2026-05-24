#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;

class ArthurRecover : public StateMachineScript
{
	DECLARE_SCRIPT(ArthurRecover)

public:
	explicit ArthurRecover(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	ArthurBossController* m_arthurController = nullptr;
	float m_stateTimer = 0.0f;
};