#pragma once

#include "ScriptAPI.h"

class ArthurDetectionAggro;
class ArthurAttackConfig;

enum class ArthurBossPhase
{
	Phase1,
	Phase2
};

class ArthurBossController : public Script
{
	DECLARE_SCRIPT(ArthurBossController)

public:
	explicit ArthurBossController(GameObject* owner);

	void Start() override;
	void drawGizmo() override;
	void Update() override; // for Debug-testing only

	FieldList getExposedFields() const override;

public:
	int m_enemyType = static_cast<int>(NavAgentProfile::EnemyGround);
	float m_combatRange = 3.0f;
	float m_moveSpeed = 1.0f;
	float m_turnSpeed = 2.0f;
	float m_intervalRepath = 0.4f;
	bool m_debugEnabled = true;

private:
	ArthurDetectionAggro* m_arthurDetectionAggro = nullptr;
	ArthurAttackConfig* m_attackConfig = nullptr;
	Transform* m_currentTarget = nullptr;

	ArthurBossPhase m_phase = ArthurBossPhase::Phase1;

	bool m_hasStartedEncounter = false;

	float m_repathTimer = 0.0f;
	std::vector<Vector3> m_path;
	bool m_hasPath = false;
	size_t m_currentIndex = 0;
	size_t m_maxPathPoints = 32;
	Vector3 m_searchExtents = Vector3(5.0f, 5.0f, 5.0f);
	const float RADIANS_TO_DEGREES = 180.0f / 3.14159265f;

	bool m_deathTriggerSent = false;

	float m_recoveryDuration = 0.75f;

	int m_selectedSideSweepSide = 1;

	// Attack Cooldown
	float m_chargingSlamCooldownTimer = 0.0f;
	float m_sideSweepCooldownTimer = 0.0f;
	float m_earthHammerCooldownTimer = 0.0f;

public:
	bool hasValidTarget() const;
	void updateCurrentTarget();
	bool isTargetInCombatRange() const;
	float getDistanceToCurrentTarget() const; // Get the distance between target and boss and compare with the different attack ranges
	Transform* getCurrentTarget() const { return m_currentTarget; }
	bool isDead() const;
	bool trySendDeathTrigger(AnimationComponent* animation);

	// Phase helpers
	void setPhase(ArthurBossPhase phase);
	ArthurBossPhase getPhase() const { return m_phase; }
	bool isPhase2() const { return m_phase == ArthurBossPhase::Phase2; }
	void updateBossPhase();

	// Attack Cooldown Helpers
	void updateAttackCooldowns(float dt);

	bool isChargingSlamReady() const { return m_chargingSlamCooldownTimer <= 0.0f; }
	bool isSideSweepReady() const { return m_sideSweepCooldownTimer <= 0.0f; }
	bool isEarthHammerReady() const { return m_earthHammerCooldownTimer <= 0.0f; }

	void consumeChargingSlamCooldown();
	void consumeSideSweepCooldown();
	void consumeEarthHammerCooldown();

	//Attack/state helpers
	Transform* getFocusTarget() const { return m_currentTarget; }
	Transform* getNonFocusTarget() const;

	void faceCurrentTarget();
	void facePosition(const Vector3& worldPosition);

	void setRecoveryDuration(float recoveryDuration);
	float getRecoveryDuration() const { return m_recoveryDuration; }

	bool areBothPlayersInEarthHammerRange() const;
	bool isTargetInChargingSlamRange() const;
	bool isCurrentTargetInsideHeavySwipeArea(float range, float halfAngleDegrees) const;

	//Needed to tell Side Sweep attack which side is the attack. We also use these in ArthurSideSweep
	bool isTargetInsideSideSweepZone(Transform* targetTransform, int side) const;
	Vector3 getSideSweepDirection(int side) const;
	bool trySelectSideSweepSide(); //This one will be used when deciding to enter SideSweep state
	int getSelectedSideSweepSide() const { return m_selectedSideSweepSide; }


	//Movement/path helpers
	void clearPath();
	bool buildPathToTarget();
	void followPath();
	void resetRepathTimer();
	void addToRepathTimer(float dt);
	bool shouldRepath() const;

private:
	Vector3 getChasePosition() const;
	void rotateTowardsDirection(const Vector3& direction);

	// Needed to detect if a player is on the area to use Side Sweep
	Vector3 rotateAroundY(const Vector3& vector, float radians) const;

public:
	void updateHealthUI();
	void setupHealthUI();
	void showHealthUI(bool show);
	void updateHealthUIPhase();

	float m_healthBarDuration = 1.0f;

private:
	ComponentRef<Transform> m_healthBarCanvas;
	ComponentRef<Transform2D> m_healthBarContainer;
	ComponentRef<Transform2D> m_healthBarPhase2;

	Transform* m_healthBarCanvasTransform = nullptr;
	Transform2D* m_healthBarContainerTransform2D = nullptr;
	Transform2D* m_healthBarPhase2Transform2D = nullptr;

	float m_healthBarTimer = 0.0f;
	bool m_healthBarVisible = false;
	float m_healthBarPhase2Timer = 0.0f;
	bool m_healthBarPhase2Visible = false;
};