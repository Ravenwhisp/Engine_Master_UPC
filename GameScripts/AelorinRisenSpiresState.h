#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

#include "AelorinBossController.h"

class AnimationComponent;
class AelorinAttackExecutor;
class AelorinUI;

class AelorinRisenSpiresState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinRisenSpiresState)

public:
	explicit AelorinRisenSpiresState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void executePattern(Transform* patternRoot, const char* sourceName);
	void finishAbility();

private:
	AelorinBossController* m_controller = nullptr;
	AelorinAttackExecutor* m_attackExecutor = nullptr;
	AnimationComponent* m_animation = nullptr;
	AelorinUI* m_aelorinUI = nullptr;

	AelorinAbility m_activeAbility = AelorinAbility::None;

	float m_stateTimer = 0.0f;

	bool m_firstPassExecuted = false;
	bool m_secondPassExecuted = false;
	bool m_completed = false;

	bool m_isFuryCast = false;
};