#pragma once

#include "EnemyBaseController.h"

class ArthurDetectionAggro;
class ArthurAttackConfig;
class ArthurUI;

enum class ArthurBossPhase
{
	Phase1,
	Phase2
};

class ArthurBossController : public EnemyBaseController
{
	DECLARE_SCRIPT(ArthurBossController)

public:
	explicit ArthurBossController(GameObject* owner);

	void Start() override;
	void drawGizmo() override;
	void Update() override;

	ScriptFieldList getExposedFields() const override;

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

	bool areBothPlayersInEarthHammerRange() const;
	bool isTargetInChargingSlamRange() const;
	bool isCurrentTargetInsideHeavySwipeArea(float range, float halfAngleDegrees) const;

	//Needed to tell Side Sweep attack which side is the attack. We also use these in ArthurSideSweep
	bool isTargetInsideSideSweepZone(Transform* targetTransform, int side) const;
	Vector3 getSideSweepDirection(int side) const;
	bool trySelectSideSweepSide(); //This one will be used when deciding to enter SideSweep state
	int getSelectedSideSweepSide() const { return m_selectedSideSweepSide; }
	
protected:
	Transform* acquireCurrentTarget() override;
	bool isTargetDowned(Transform* target) const override;

private:
	// Needed to detect if a player is on the area to use Side Sweep
	Vector3 rotateAroundY(const Vector3& vector, float radians) const;

public:
	float m_combatRange = 3.0f;

private:
	ArthurDetectionAggro* m_arthurDetectionAggro = nullptr;
	ArthurAttackConfig* m_attackConfig = nullptr;
	ArthurUI* m_arthurUI = nullptr;

	ArthurBossPhase m_phase = ArthurBossPhase::Phase1;

	bool m_hasStartedEncounter = false;

	const float RADIANS_TO_DEGREES = 180.0f / 3.14159265f;

	int m_selectedSideSweepSide = 1;

	// Attack Cooldown
	float m_chargingSlamCooldownTimer = 0.0f;
	float m_sideSweepCooldownTimer = 0.0f;
	float m_earthHammerCooldownTimer = 0.0f;
};