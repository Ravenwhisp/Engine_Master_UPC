#pragma once

#include "ScriptAPI.h"

class EnemyDetectionAggro : public Script
{
	DECLARE_SCRIPT(EnemyDetectionAggro)

protected:
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

	FieldList getExposedFields() const override;

public:
	float m_detectionRadius = 10.0f;
	float m_targetLockDuration = 5.0f;
	bool m_debugEnabled = true;

	ComponentRef<Transform> m_lyrielTransform;
	ComponentRef<Transform> m_deathTransform;

public:
	void notifyPlayerAttackedEnemy(Transform* playerTransform);
	void applyTaunt(Transform* playerTransform, float durationSeconds);
	void clearTaunt(Transform* playerTransform);

	bool isAggro() const { return m_isAggro; }
	bool canSeeTarget() const { return m_canSeeTarget; }

	Transform* getCurrentTarget() const { return m_currentTargetTransform; }
	Vector3 getLastKnownTargetPosition() const { return m_lastKnownTargetPosition; }

	bool isDowned(Transform* target) const;
	bool hasAnyTargetInDetectionRange();

	Transform* getLyrielTransform() const;
	Transform* getDeathTransform() const;

protected:
	AggroEntry m_lyrielAggro;
	AggroEntry m_deathAggro;

	Transform* m_currentTargetTransform = nullptr;
	bool m_isAggro = false;
	bool m_canSeeTarget = false;
	Vector3 m_lastKnownTargetPosition = Vector3(0.0f, 0.0f, 0.0f);
	Transform* m_tauntTargetTransform = nullptr;
	float m_tauntTimer = 0.0f;

	float m_currentTargetLockTimer = 0.0f;
	float m_currentTime = 0.0f;
	float m_recentAttackMemory = 3.0f;

protected:
	void enterAggro(Transform* target);
	virtual void updateAggroState();
	void updateAggroEntries();
	void resetAggro();
	bool isTaunted() const;


private:
	bool isTargetLockActive() const;
	void startTargetLock();
	void updateTargetLockTimer();
	void updateTauntTimer();
	void findPlayerTransforms();

	Transform* selectClosestDetectedPlayer() const;
	Transform* selectReevaluatedTarget() const;

	Transform* getOwnerTransform() const;
	Vector3 getOwnerPosition() const;
	Vector3 getLyrielPosition() const;
	Vector3 getDeathPosition() const;

	float getDistanceToLyriel() const;
	float getDistanceToDeath() const;

	bool isLyrielInDetectionRange() const;
	bool isDeathInDetectionRange() const;

	bool isLyrielAggroing() const;
	bool isDeathAggroing() const;

	AggroEntry* getAggroEntry(Transform* target);
	const AggroEntry* getAggroEntry(Transform* target) const;

	Transform* m_lyrielCachedTransform = nullptr;
	Transform* m_deathCachedTransform = nullptr;
};