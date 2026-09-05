#pragma once

#include "EnemyBaseController.h"

class ArthurDetectionAggro;
class ArthurAttackConfig;
class ArthurUI;
class Damageable;
class ArthurSound;
class CameraShake;
class CameraTransitionEvent;
class CameraTransitionController;

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

	FieldList getExposedFields() const override;

	bool isSeparationEnabled() const override { return false; }

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
	const EnemyBaseDataConfig* getBaseDataConfig() const override;
	Transform* acquireCurrentTarget() override;
	bool isTargetDowned(Transform* target) const override;

private:
	// Needed to detect if a player is on the area to use Side Sweep
	Vector3 rotateAroundY(const Vector3& vector, float radians) const;

	// Fires the referenced boss cinematic (encounter / defeat) if its ref is set. Returns true if a
	// cinematic was started. Plays it as a full cinematic, same as the Paladin / banquet room
	// transitions (the event's own Lock / Invulnerable / Fade HUD flags decide whether it pauses).
	bool playCinematic(const ComponentRef<Transform>& cinematicRef);

	// Encounter intro: Arthur stays idle while the cinematic plays; combat only starts once the
	// camera transition returns, so he never lunges mid-cinematic.
	void beginEncounterCombat();
	bool isCinematicRunning() const;

	// Moves the defeat cinematic's first camera point to where Arthur actually died (not his spawn).
	void repositionDeathShot();

public:
	float m_combatRange = 3.0f;
	AssetReference<ArthurAttackConfig> m_attackConfig;

	// Cinematics fired by the boss (no trigger zone). Each ref points at the Transform of the
	// GameObject that holds the CameraTransitionEvent (+ its CameraPoints child).
	ComponentRef<Transform> m_encounterCinematic;
	ComponentRef<Transform> m_defeatCinematic;

	// Seconds into the intro cinematic before Arthur roars (tune so it lands when the camera
	// reaches him).
	float m_encounterRoarDelay = 4.0f;

	// Camera offset from Arthur's death position for the defeat close-up (slight zoom).
	Vector3 m_deathCamOffset = Vector3(7.0f, 10.0f, -7.0f);

private:
	ArthurDetectionAggro* m_arthurDetectionAggro = nullptr;
	ArthurUI* m_arthurUI = nullptr;

	ArthurBossPhase m_phase = ArthurBossPhase::Phase1;

	bool m_hasStartedEncounter = false;

	// Música: al morir Arthur se vuelve a Level1_Chapel (una sola vez).
	bool m_bossDefeated = false;
	Damageable* m_damageable = nullptr;
	ArthurSound* m_arthurSound = nullptr;
	CameraShake* m_cameraShake = nullptr;
	CameraTransitionController* m_cameraTransition = nullptr;

	// Intro cinematic runtime state.
	bool m_encounterCinematicPlaying = false;
	float m_encounterTimer = 0.0f;
	bool m_encounterRoarPlayed = false;

	const float RADIANS_TO_DEGREES = 180.0f / 3.14159265f;

	int m_selectedSideSweepSide = 1;

	// Attack Cooldown
	float m_chargingSlamCooldownTimer = 0.0f;
	float m_sideSweepCooldownTimer = 0.0f;
	float m_earthHammerCooldownTimer = 0.0f;
};