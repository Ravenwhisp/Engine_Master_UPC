#pragma once

#include "ScriptAPI.h"

class EnemyDeathState;

class LinkedDeathController : public Script
{
	DECLARE_SCRIPT(LinkedDeathController)

public:
	explicit LinkedDeathController(GameObject* owner);

	void Start() override;
	void Update() override;

	ScriptFieldList getExposedFields() const override;

	void notifyLinkedDeath();

public:
	ScriptComponentRef<Transform> m_linkedPartner;
	float m_graceWindow = 3.0f;
	float m_reviveHpPercent = 0.5f;

private:
	enum class State
	{
		Inactive,
		EnteringDeath,
		Waiting,
		ProceedingToDeath,
		Reviving
	};

	void findPartnerController();
	bool isPartnerDead() const;
	void reviveAndExit();

	EnemyDeathState* m_deathState = nullptr;
	LinkedDeathController* m_partnerController = nullptr;
	State m_state = State::Inactive;
	float m_pendingDeathTimer = 0.0f;
	bool m_partnerDiedNotification = false;
};
