#pragma once

#include "Damageable.h"

class EnemyDetectionAggro;

enum class EnemyAttackType
{
	None = 0,
	DeathBasic,
	DeathCharged,
	DeathDash,
	LyrielArrow,
	LyrielVolley,
	LyrielCharged,
	ShadowExecution,
	Environment
};

struct EnemyHitContext : public HitContext
{
	Transform* attacker = nullptr;
	EnemyAttackType attackType = EnemyAttackType::None;
};

class EnemyDamageable : public Damageable
{
	DECLARE_SCRIPT(EnemyDamageable)

	public:
		explicit EnemyDamageable(GameObject* owner);

		void Start() override;
		void takeDamage(const HitContext& ctx) override;

	protected:
		void onDamaged(float amount) override;

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
	Transform* m_damageSource = nullptr;
};
