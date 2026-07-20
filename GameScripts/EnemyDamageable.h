#pragma once

#include "Damageable.h"

class EnemyDetectionAggro;
class EnemySound;
class Transform2D;

enum class PlayerAttackType
{
	None = 0,
	DeathBasic,
	DeathCharged,
	DeathDash,
	DeathTaunt,
	LyrielArrow,
	LyrielVolley,
	LyrielCharged,
	ShadowExecution,
	ShadowMarkExploit,
	Environment
};

struct EnemyHitContext : public HitContext
{
	Transform* attacker = nullptr;
	PlayerAttackType attackType = PlayerAttackType::None;
};

class EnemyDamageable : public Damageable
{
	DECLARE_SCRIPT(EnemyDamageable)

public:
	explicit EnemyDamageable(GameObject* owner);

	void Start() override;
	void Update() override;
	void takeDamage(const HitContext& ctx) override;

	FieldList getExposedFields() const override;

protected:
	void onDamaged(float amount) override;

private: 
	void resolveHealthBarReferences();
	void updateHealthBarFade();
	void setHealthBarAlpha(float alpha);

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
	EnemySound* m_enemySound = nullptr;
	Transform* m_damageSource = nullptr;

	ComponentRef<Transform2D> m_healthBarContainer;
	Transform2D* m_healthBarContainerTransform = nullptr;

	float m_healthBarFadeTime = 0.25f;
	float m_healthBarFadeTimer = 0.0f;
	bool m_healthBarFadeActive = false;
};
