#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

#include "AelorinBossController.h"

class AnimationComponent;
class AelorinUI;

class AelorinNovaState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinNovaState)

public:
	explicit AelorinNovaState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void executeFirstNovaWave();
	void executeSecondNovaWave();
	void executeNovaWave(float radius, float damage);
	void finishAbility();

private:
	AelorinBossController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;
	AelorinUI* m_aelorinUI = nullptr;

	AelorinAbility m_activeAbility = AelorinAbility::None;

	float m_stateTimer = 0.0f;

	Vector3 m_novaCenter = Vector3::Zero;
	bool m_firstWaveApplied = false;
	bool m_secondWaveApplied = false;
	bool m_completed = false;

	bool m_isFuryCast = false;
};