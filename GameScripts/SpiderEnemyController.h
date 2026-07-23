#pragma once

#include "EnemyBaseController.h"

class EnemyDetectionAggro;
class EnemyBaseAttackConfig;

class SpiderEnemyController : public EnemyBaseController
{
	DECLARE_SCRIPT(SpiderEnemyController)

public:
	explicit SpiderEnemyController(GameObject* owner);

	void Start() override;
	void Update() override;
	FieldList getExposedFields() const override;

	const EnemyBaseAttackConfig* getAttackConfig() const override;

protected:
	Transform* acquireCurrentTarget() override;
	bool isTargetDowned(Transform* target) const override;

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;

public:
	AssetReference<EnemyBaseAttackConfig> m_attackConfig;
};