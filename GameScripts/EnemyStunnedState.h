#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyBaseController;
class AnimationComponent;
class EnemyStunParticles;
class EnemySound;

class EnemyStunnedState : public StateMachineScript
{
	DECLARE_SCRIPT(EnemyStunnedState)

public:
	EnemyStunnedState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	EnemyBaseController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;
	EnemySound* m_enemySound = nullptr;
    EnemyStunParticles*   m_stunParticles = nullptr;
	float m_stateTimer = 0.0f;
};