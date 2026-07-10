#pragma once

#include "EnemyDamageable.h"

class SkeletonEnemyController;
class SkeletonAttackConfig;
class Transform2D;

class SkeletonDamageable final : public EnemyDamageable
{
	DECLARE_SCRIPT(SkeletonDamageable)

public:
	explicit SkeletonDamageable(GameObject* owner);

	void Start() override;

	FieldList getExposedFields() const override;

	void takeDamage(const HitContext& ctx) override;
	
	bool isDowned() const;
	bool isPermanentlyDead() const;

	void completeRevive();

	void cacheHealthBarBackgroundTransform();
	void applyHealthBarScaleForState();

protected:
	void onHpDepleted() override;

private:
	enum class SkeletonLifeState
	{
		Alive,
		Downed,
		PermanentlyDead
	};

	bool shouldBlockDamage(const EnemyHitContext& enemyCtx) const;
	void startDowned();
	void confirmKill();

private:
	SkeletonEnemyController* m_skeletonController = nullptr;
    AssetRef<SkeletonAttackConfig> m_attackConfig;

	Transform2D* m_healthBarBackgroundTransform2D = nullptr;
	Vector2 m_originalHealthBarScale = Vector2(1.0f, 1.0f);
	Vector2 m_downedHealthBarScale = Vector2(0.6f, 1.0f);

	SkeletonLifeState m_lifeState = SkeletonLifeState::Alive;
	float m_previousMaxHp = 0.0f;
};