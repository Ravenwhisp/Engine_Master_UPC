#pragma once

#include "ScriptAPI.h"
#include "EnemyDamageable.h"

#include <functional>

class EnemyBaseController;
class Transform;

// This class can be reused if there is any other forced movement on the enemies besides Death Taunt Pull
class EnemyForcedMovement : public Script
{
	DECLARE_SCRIPT(EnemyForcedMovement)

public:
	explicit EnemyForcedMovement(GameObject* owner);

	void Start() override;
	void Update() override;

	bool startPull(const Vector3& destination, float duration, Transform* attacker, float completionDamage, PlayerAttackType attackType);
	void cancelPull();

	bool isBeingPulled() const { return m_isBeingPulled; }

	// Called once when the current pull ends (finished or cancelled), then cleared.
	// Set it after a successful startPull().
	using PullFinishedCallback = std::function<void(GameObject* enemy)>;
	void setPullFinishedCallback(PullFinishedCallback callback) { m_onPullFinished = std::move(callback); }

private:
	void updatePull();
	void finishPull();
	void applyCompletionDamage();
	void clearPullData();
	void notifyPullFinished();

private:
	Transform* m_transform = nullptr;
	EnemyBaseController* m_controller = nullptr;
	EnemyDamageable* m_damageable = nullptr;

	Transform* m_attacker = nullptr;

	Vector3 m_startPosition = Vector3::Zero;
	Vector3 m_destination = Vector3::Zero;

	float m_elapsedTime = 0.0f;
	float m_duration = 0.0f;
	float m_completionDamage = 0.0f;

	PlayerAttackType m_attackType = PlayerAttackType::None;

	bool m_isBeingPulled = false;

	PullFinishedCallback m_onPullFinished;
};