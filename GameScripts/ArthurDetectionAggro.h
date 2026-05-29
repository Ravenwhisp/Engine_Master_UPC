#pragma once

#include "EnemyDetectionAggro.h"

class ArthurBossController;

class ArthurDetectionAggro : public EnemyDetectionAggro
{
	DECLARE_SCRIPT(ArthurDetectionAggro)

public:
	explicit ArthurDetectionAggro(GameObject* owner);

	void Start() override;

public:
	void startEncounter(); // called in controller
	void stopEncounter(); // called in controller

private:
	ArthurBossController* m_arthurBossController = nullptr;
	bool m_encounterStarted = false;

protected:
	void updateAggroState() override;
};