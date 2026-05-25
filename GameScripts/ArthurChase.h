#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class ArthurAttackConfig;

class ArthurChase : public StateMachineScript
{
	DECLARE_SCRIPT(ArthurChase)

public:
	explicit ArthurChase(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	ArthurBossController* m_arthurController = nullptr;
	ArthurAttackConfig* m_arthurAttackConfig = nullptr;
};