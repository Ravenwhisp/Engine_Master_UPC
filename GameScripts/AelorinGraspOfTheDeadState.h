#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

#include "AelorinBossController.h"

class AnimationComponent;
class AelorinAttackExecutor;
class PlayerMovement;
class AelorinUI;

class AelorinGraspOfTheDeadState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinGraspOfTheDeadState)

public:
	explicit AelorinGraspOfTheDeadState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void pullPlayer(Transform* playerTransform, PlayerMovement* playerMovement);

	void chainIntoNova();

private:
	AelorinBossController* m_controller = nullptr;
	AelorinAttackExecutor* m_attackExecutor = nullptr;
	AnimationComponent* m_animation = nullptr;
	AelorinUI* m_aelorinUI = nullptr;

	PlayerMovement* m_lyrielMovement = nullptr;
	PlayerMovement* m_deathMovement = nullptr;

	AelorinAbility m_activeAbility = AelorinAbility::None;

	float m_stateTimer = 0.0f;
	bool m_completed = false;

	bool m_isFuryCast = false;
};