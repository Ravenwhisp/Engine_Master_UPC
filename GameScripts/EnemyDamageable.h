#pragma once

#include "Damageable.h"
#include "PlayerAttackType.h"

class EnemyDetectionAggro;
class EnemySound;
class EnemyShadowMark;
class Transform2D;
class EnemyBaseController;

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

	FieldList getExposedFields() const override;
	
    void takeDamage(const HitContext& ctx) override;
	bool lastHitExploitShadowMark() const { return m_lastHitExploitedShadowMark; }

protected:
	void onDamaged(float amount) override;
	void onDeath() override;

	void resetLastShadowMarkResult() { m_lastHitExploitedShadowMark = false; }
	bool processShadowMarkHit(PlayerAttackType attackType);
	void applyDamageWithoutShadowMark(const EnemyHitContext& hit);

	virtual void setHealthBarAlpha(float alpha);

private: 
	void resolveHealthBarReferences();
	void updateHealthBarFade();

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
	EnemySound* m_enemySound = nullptr;
	EnemyShadowMark* m_shadowMark = nullptr;
	Transform* m_damageSource = nullptr;
	
	bool m_lastHitExploitedShadowMark = false;

	ComponentRef<Transform2D> m_healthBarContainer;
	Transform2D* m_healthBarContainerTransform = nullptr;

	float m_healthBarFadeTime = 0.25f;
	float m_healthBarFadeTimer = 0.0f;
	bool m_healthBarFadeActive = false;
};
