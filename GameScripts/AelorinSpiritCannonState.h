#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

#include "AelorinBossController.h"

class AnimationComponent;
class AelorinAttackExecutor;
class AelorinUI;

class AelorinSpiritCannonState : public StateMachineScript
{
	DECLARE_SCRIPT(AelorinSpiritCannonState)

public:
	explicit AelorinSpiritCannonState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	void selectLockedTarget();
	bool isValidTarget(Transform* targetTransform) const;
	void ensureValidLockedTarget();

	void initializeAimDirection();
	void updateAimDirection(float trackingSpeed);

	void fireBeamShot(float width, float damage, const char* sourceName);
	
	void finishAbility();

private:
	AelorinBossController* m_controller = nullptr;
	AelorinAttackExecutor* m_attackExecutor = nullptr;
	AnimationComponent* m_animation = nullptr;
	AelorinUI* m_aelorinUI = nullptr;

	Transform* m_aelorinTransform = nullptr;
	Transform* m_lockedTarget = nullptr;
	Vector3 m_currentAimDirection = Vector3::Zero;

	AelorinAbility m_activeAbility = AelorinAbility::None;

	float m_stateTimer = 0.0f;
	int m_shotCount = 0;
	bool m_completed = false;

	bool m_isFuryCast = false;
};