#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

#include "AelorinBossController.h"

class AnimationComponent;
class ProjectilePool;
class AelorinUI;

class AelorinSeekerSigilsState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinSeekerSigilsState)

public:
	explicit AelorinSeekerSigilsState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	AelorinBossController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;
	ProjectilePool* m_normalProjectilePool = nullptr;
	ProjectilePool* m_largeProjectilePool = nullptr;
	AelorinUI* m_aelorinUI = nullptr;

	AelorinAbility m_activeAbility = AelorinAbility::None;

	float m_waveTimer = 0.0f;
	int m_currentWave = 0;

	bool m_finalProjectileLaunched = false;
	bool m_completed = false;

	bool m_isFuryCast = false;

private:
	void launchCurrentWave();
	void launchProjectileAt(ProjectilePool* projectilePool, const Vector3& targetPosition, float impactRadius, float damage);

	void launchPhase2FinalProjectile();
	void finishAbility();
};