#pragma once

#include "ScriptAPI.h"

class AelorinSummonSlot : public Script
{
	DECLARE_SCRIPT(AelorinSummonSlot)

public:
	explicit AelorinSummonSlot(GameObject* owner);

	FieldList getExposedFields() const override;

	bool hasLivingEnemy() const;
	GameObject* spawnEnemy();

public:
	PrefabRef m_enemyPrefab;
	std::string m_legacyEnemyPath;
};