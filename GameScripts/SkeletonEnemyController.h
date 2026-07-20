#pragma once

#include "EnemyBaseController.h"

class EnemyDetectionAggro;
class SkeletonAttackConfig;
class SkeletonDamageable;

class SkeletonEnemyController : public EnemyBaseController
{
	DECLARE_SCRIPT(SkeletonEnemyController)

public:
	explicit SkeletonEnemyController(GameObject* owner);

	void Start() override;
	void Update() override;
	FieldList getExposedFields() const override;
	bool isTargetInScimitarRange() const;

	bool isGuardReady() const;
	void consumeGuardCooldown();
	void updateGuardCooldown(float dt);
	bool shouldUseGuard() const;
	bool isGuarding() const;
	void setGuarding(bool guarding);

	bool isDowned() const;
	bool isPermanentlyDead() const;

	bool trySendReviveTrigger(AnimationComponent* animation);

protected:
	Transform* acquireCurrentTarget() override;
	bool isTargetDowned(Transform* target) const override;

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
	AssetReference<SkeletonAttackConfig> m_attackConfig;
	SkeletonDamageable* m_damageable = nullptr;

	float m_guardCooldownTimer = 0.0f;
	bool m_isGuarding = false;
};