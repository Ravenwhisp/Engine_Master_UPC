#pragma once

#include "EnemyBaseController.h"

class EnemyDetectionAggro;
class PaladinAttackConfig;

class MeleeEnemyController : public EnemyBaseController
{
	DECLARE_SCRIPT(MeleeEnemyController)

public:
	explicit MeleeEnemyController(GameObject* owner);
	FieldList getExposedFields() const override;

	void Start() override;
	void Update() override;

	bool isTargetInAttackRange() const;

	// Charge helpers
	bool playerInChargeRange() const;

	bool isChargeReady() const;
	void consumeChargeCooldown();
	void updateChargeCooldown(float dt);

	Vector3 getChargeDirection() const;

protected:
	Transform* acquireCurrentTarget() override;
	bool isTargetDowned(Transform* target) const override;

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
    AssetReference<PaladinAttackConfig> m_attackConfig;

	float m_chargeCooldownTimer = 0.0f;
};