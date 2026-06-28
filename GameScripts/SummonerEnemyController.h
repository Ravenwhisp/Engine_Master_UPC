#pragma once

#include "EnemyBaseController.h"

class EnemyDetectionAggro;
class SummonerAttackConfig;
class Transform;

class SummonerEnemyController : public EnemyBaseController
{
	DECLARE_SCRIPT(SummonerEnemyController)

public:
	explicit SummonerEnemyController(GameObject* owner);

	void Start() override;
	void Update() override;

	bool isTargetInAttackRange() const;

	bool isTeleportReady() const;
	void consumeTeleportCooldown();
	bool tryGetTeleportPosition(Vector3& outPosition) const;

	bool isSummonReady() const;
	void consumeSummonCooldown();
	void summonSpidersAroundSelf();

	float getRecoveryDuration() const;

	bool isAttackReady() const;
	void consumeAttackCooldown();

protected:
	Transform* acquireCurrentTarget() override;
	bool isTargetDowned(Transform* target) const override;

private:
	void updateTeleportCooldown(float dt);
	void updateSummonCooldown(float dt);
	void updateAttackCooldown(float dt);

private:
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
	SummonerAttackConfig* m_attackConfig = nullptr;

	float m_attackCooldownTimer = 0.0f;
	float m_teleportCooldownTimer = 0.0f;
	float m_summonCooldownTimer = 0.0f;
};