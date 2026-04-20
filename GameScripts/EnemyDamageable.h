#pragma once

#include "Damageable.h"

class EnemyDetectionAggro;

class EnemyDamageable : public Damageable
{
	DECLARE_SCRIPT(EnemyDamageable)

public:
	explicit EnemyDamageable(GameObject* owner);

	void Start() override;
	void takeDamageEnemy(float amount, Transform* playerTransform);

protected:
	void onDamaged(float amount) override;

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
	Transform* m_damageSource = nullptr;
};