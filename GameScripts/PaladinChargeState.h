#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class MeleeEnemyController;
class AnimationComponent;
class PaladinSound;
class PaladinVFX;

class PaladinChargeState : public StateMachineScript
{
	DECLARE_SCRIPT(PaladinChargeState)

public:
	explicit PaladinChargeState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void moveCharge();
	void finishCharge();
	void cancelCharge();

	void stopChargeAttackEffect();

private:
	MeleeEnemyController* m_paladinController = nullptr;
	AnimationComponent* m_animation = nullptr;
	PaladinSound* m_paladinSound = nullptr;
	PaladinVFX* m_paladinVFX = nullptr;

	Vector3 m_chargeDirection = Vector3::Zero;

	float m_stateTimer = 0.0f;
};