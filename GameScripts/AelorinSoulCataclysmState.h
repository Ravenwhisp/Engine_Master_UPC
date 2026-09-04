#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class AelorinBossController;
class AelorinAttackExecutor;
class AnimationComponent;
class AelorinUI;
class AelorinLavaController;

class AelorinSoulCataclysmState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinSoulCataclysmState)

public:
	explicit AelorinSoulCataclysmState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void executeCataclysm();
	void damagePlayerIfUnsafe(Transform* playerTransform);
	bool isPlayerInsideSafeZone(Transform* playerTransform) const;
	void finishCataclysm();

private:
	AelorinBossController* m_controller = nullptr;
	AelorinAttackExecutor* m_attackExecutor = nullptr;
	AnimationComponent* m_animation = nullptr;
	AelorinUI* m_aelorinUI = nullptr;
	AelorinLavaController* m_lavaController = nullptr;

	float m_stateTimer = 0.0f;

	bool m_cataclysmExecuted = false;
	bool m_completed = false;
};