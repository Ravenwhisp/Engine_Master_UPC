#pragma once

#include "EnemyBaseController.h"

class EnemyDetectionAggro;
class EnemyBaseAttackConfig;

class SpiderEnemyController : public EnemyBaseController
{
	DECLARE_SCRIPT(SpiderEnemyController)

public:
	explicit SpiderEnemyController(GameObject* owner);
	FieldList getExposedFields() const override;

	void Start() override;
	void Update() override;

	bool isTargetInAttackRange() const;

protected:
	Transform* acquireCurrentTarget() override;
	bool isTargetDowned(Transform* target) const override;

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
    AssetReference<EnemyBaseAttackConfig> m_attackConfig;
};