#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

#include "AelorinBossController.h"

class AnimationComponent;

class AelorinSummonState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinSummonState)

public:
	explicit AelorinSummonState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void executeSummon();

	int fillFormation(Transform* formationRoot, int maxToSpawn);

	void finishAbility();

private:
	AelorinBossController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;

	AelorinAbility m_activeAbility = AelorinAbility::None;

	float m_stateTimer = 0.0f;
	float m_recoveryTimer = 0.0f;

	bool m_summonExecuted = false;
	bool m_completed = false;
};