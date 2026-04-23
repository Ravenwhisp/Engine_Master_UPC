#pragma once

#include "ScriptAPI.h"

class EnemyDetectionAggro : public Script
{
	DECLARE_SCRIPT(EnemyDetectionAggro)

private:
	struct AggroEntry
	{
		Transform* targetTransform = nullptr;
		bool isInDetectionRange = false;
		float distanceToEnemy = 0.0f;
		float lastAttackTime = -9999.9f;
	};

public:
	explicit EnemyDetectionAggro(GameObject* owner);

	void Start() override;
	void Update() override;
	void drawGizmo() override;

	ScriptFieldList getExposedFields() const override;

public:
	float m_detectionRadius = 10.0f;
	float m_targetLockDuration = 5.0f;
	bool m_debugEnabled = true;

	ScriptComponentRef<Transform> m_player1Transform;
	ScriptComponentRef<Transform> m_player2Transform;

public:
	void notifyPlayerAttackedEnemy(Transform* playerTransform);
	void applyTaunt(Transform* playerTransform, float durationSeconds);
	void clearTaunt(Transform* playerTransform);

	bool isAggro() const { return m_isAggro; }
	bool canSeeTarget() const { return m_canSeeTarget; }
	Transform* getCurrentTarget() const { return m_currentTargetTransform; }
	Vector3 getLastKnownTargetPosition() const { return m_lastKnownTargetPosition; }

private:
	AggroEntry m_player1Aggro;
	AggroEntry m_player2Aggro;

	Transform* m_currentTargetTransform = nullptr;
	bool m_isAggro = false;
	bool m_canSeeTarget = false;
	Vector3 m_lastKnownTargetPosition = Vector3(0.0f, 0.0f, 0.0f);
	Transform* m_tauntTargetTransform = nullptr;
	float m_tauntTimer = 0.0f;

	float m_currentTargetLockTimer = 0.0f;
	float m_currentTime = 0.0f;
	float m_recentAttackMemory = 3.0f;

private:
	void enterAggro(Transform* target);
	void updateAggroState();
	void updateAggroEntries();

	bool isTargetLockActive() const;
	void startTargetLock();
	void updateTargetLockTimer();
	void updateTauntTimer();
	bool isTaunted() const;

	Transform* selectClosestDetectedPlayer() const;
	Transform* selectReevaluatedTarget() const;

private:
	Transform* getOwnerTransform() const;
	Transform* getPlayer1Transform() const;
	Transform* getPlayer2Transform() const;

	Vector3 getOwnerPosition() const;
	Vector3 getPlayer1Position() const;
	Vector3 getPlayer2Position() const;

	float getDistanceToPlayer1() const;
	float getDistanceToPlayer2() const;

	bool isPlayer1InDetectionRange() const;
	bool isPlayer2InDetectionRange() const;

	bool isPlayer1Aggroing() const;
	bool isPlayer2Aggroing() const;
	bool isTransformAlive(Transform* target) const;

	AggroEntry* getAggroEntry(Transform* target);
	const AggroEntry* getAggroEntry(Transform* target) const;
};