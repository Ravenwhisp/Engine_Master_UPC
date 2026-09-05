#pragma once

#include "EnemyBaseController.h"

class AelorinDetectionAggro;
class AelorinDamageable;
class AelorinAttackConfig;
class ProjectilePool;
class AelorinAttackExecutor;

enum class Phase
{
	Phase1,
	Phase2
};

enum class AelorinAbility
{
	None,

	SeekerSigils,
	Nova,
	RisenSpires,
	SpiritCannon,
	GraspOfTheDead,

	Summon,
	Teleport
};

class AelorinBossController : public EnemyBaseController
{
	DECLARE_SCRIPT(AelorinBossController)

public:
	explicit AelorinBossController(GameObject* owner);

	void Start() override;
	//void drawGizmo() override;
	void Update() override;

	FieldList getExposedFields() const override;

	Transform* getLyrielTransform() const;
	Transform* getDeathTransform() const;

	Vector3 getLyrielPosition() const;
	Vector3 getDeathPosition() const;
	float getClosestPlayerDistance() const;

	// Projectile Pool References
	ComponentRef<Transform> m_seekerSigilsProjectilePool;
	ComponentRef<Transform> m_seekerSigilsLargeProjectilePool;

	ProjectilePool* getSeekerSigilsProjectilePool() const { return m_seekerSigilsProjectilePoolScript; }
	ProjectilePool* getSeekerSigilsLargeProjectilePool() const { return m_seekerSigilsLargeProjectilePoolScript; }

	// AttackConfig
	const AelorinAttackConfig* getAelorinAttackConfig() const { return m_attackConfig.get(); }

	// Attack Executor
	AelorinAttackExecutor* getAttackExecutor() const { return m_attackExecutor; }

	// Ability Choosing
	AelorinAbility chooseNextAbility(); // normal ability
	AelorinAbility chooseNextFuryAbility(); // fury ability
	AelorinAbility consumeRequestedAbility();
	bool requestAbility(AelorinAbility ability);

	bool hasRequestedAbility() const { return m_requestedAbility != AelorinAbility::None; }
	AelorinAbility getRequestedAbility() const { return m_requestedAbility; }
	bool trySendRequestedAbilityTrigger(AnimationComponent* animation);
	void clearRequestedAbility();

	// Encounter
	void updateEncounter();
	bool hasEncounterStarted() const { return m_hasStartedEncounter; }

	// Decision Timing helpers
	float getDecisionTime() const;

	// Phase helpers
	Phase getPhase() const { return m_phase; }
	void setPhase(Phase phase);
	bool isPhase2() const { return m_phase == Phase::Phase2; }
	
	void requestPhaseTransition();
	void markPhaseTransitionTriggered();
	void beginPhase2();
	bool isPhaseTransitionRequested() const { return m_phaseTransitionRequested; }
	bool canTriggerPhaseTransition() const { return m_phaseTransitionRequested && !m_phaseTransitionTriggered; }

	bool trySendPhaseTransitionTrigger(AnimationComponent* animation);

	// Threshold Stagger helpers
	void requestThresholdStagger();
	bool trySendThresholdStaggerTrigger(AnimationComponent* animation);
	void completeThresholdStagger();
	float getThresholdStaggerDuration() const;

	// Risen Spires helpers
	Transform* getRisenSpiresPatternARoot() const { return m_risenSpiresPatternARoot.getReferencedComponent(); }
	Transform* getRisenSpiresPatternBRoot() const { return m_risenSpiresPatternBRoot.getReferencedComponent(); }

	// Spirit Cannon Debug Draw helpers
	void setSpiritCannonDebugLine(const Vector3& origin, const Vector3& direction, float width);
	void clearSpiritCannonDebugLine();

	bool hasSpiritCannonDebugLine() const { return m_hasSpiritCannonDebugLine; }
	const Vector3& getSpiritCannonDebugOrigin() const { return m_spiritCannonDebugOrigin; }
	const Vector3& getSpiritCannonDebugDirection() const { return m_spiritCannonDebugDirection; }
	float getSpiritCannonDebugWidth() const { return m_spiritCannonDebugWidth; }

	// Grasp of the Dead
	Transform* getGraspCenter() const { return m_graspCenter.getReferencedComponent(); }

	// Nova helpers
	void prepareForcedNovaAt(const Vector3& center);
	bool hasNovaCenterOverride() const { return m_hasNovaCenterOverride; }
	Vector3 consumeNovaCenterOverride();

	// Teleport helpers
	Transform* getTeleportAnchorsRoot() const { return m_teleportAnchorsRoot.getReferencedComponent(); }
	Transform* getTeleportCrowdingPlayer() const;
	Transform* chooseTeleportAnchor(Transform* crowdingPlayer) const;
	bool isTeleportReady() const { return m_teleportCooldownRemaining <= 0.0f; }
	void startTeleportCooldown();
	void updateTeleportCooldown();
	float getTeleportCooldownRemaining() const { return m_teleportCooldownRemaining; }

	// Summon helpers
	Transform* getPhase1SummonFormation() const	{ return m_phase1SummonFormation.getReferencedComponent(); }
	Transform* getPhase2SummonFormation() const	{ return m_phase2SummonFormation.getReferencedComponent(); }

	int getLivingSummonCount() const;
	int getCurrentSummonCap() const;

	bool isSummonReady() const { return m_summonTimerRemaining <= 0.0f; }
	void startSummonTimer();
	float getSummonTimerRemaining() const { return m_summonTimerRemaining; }

	// Fury helpers
	void requestFury();
	void beginFury();
	void recordFuryCast();
	void finishFury();
	bool isFuryRequested() const { return m_furyRequested; }
	bool isFuryActive() const { return m_furyActive; }
	bool isFuryBarrageComplete() const;
	int getFuryCastTarget() const;
	int getFuryCastsCompleted() const { return m_furyCastsCompleted; }

	// Soul Cataclysm helpers
	bool trySendSoulCataclysmTrigger(AnimationComponent* animation);
	Transform* getSoulCataclysmCenter() const { return m_soulCataclysmCenter.getReferencedComponent(); }

	// Health drop
	void spawnHealthDrops();

protected:
	Transform* acquireCurrentTarget() override;
	bool isTargetDowned(Transform* target) const override;

public:
	AssetReference<AelorinAttackConfig> m_attackConfig;

private:
	AelorinDetectionAggro* m_aelorinDetectionAggro = nullptr;
	AelorinDamageable* m_damageable = nullptr;
	AelorinAttackExecutor* m_attackExecutor = nullptr;

	ProjectilePool* m_seekerSigilsProjectilePoolScript = nullptr;
	ProjectilePool* m_seekerSigilsLargeProjectilePoolScript = nullptr;

	// Phase
	Phase m_phase = Phase::Phase1;
	bool m_phaseTransitionRequested = false;
	bool m_phaseTransitionTriggered = false;

	// Encounter
	bool m_hasStartedEncounter = false;

	// Threshold Stagger
	bool m_thresholdStaggerRequested = false;
	bool m_thresholdStaggerTriggered = false;

	// Model switching
	GameObject* m_phase1GameObject = nullptr;
	GameObject* m_phase2GameObject = nullptr;

	// Ability
	AelorinAbility m_requestedAbility = AelorinAbility::None;
	AelorinAbility m_lastUsedAbility = AelorinAbility::None;
	bool m_abilityTriggerSent = false;

	// Risen Spires Ability
	ComponentRef<Transform> m_risenSpiresPatternARoot;
	ComponentRef<Transform> m_risenSpiresPatternBRoot;

	// Grasp of the Dead Ability
	ComponentRef<Transform> m_graspCenter;

	// Nova ability
	bool m_hasNovaCenterOverride = false;
	Vector3 m_novaCenterOverride = Vector3::Zero;

	// Teleport
	ComponentRef<Transform> m_teleportAnchorsRoot;
	float m_teleportCooldownRemaining = 0.0f;

	// Summon
	ComponentRef<Transform> m_phase1SummonFormation;
	ComponentRef<Transform> m_phase2SummonFormation;
	float m_summonTimerRemaining = 0.0f;
	int countLivingSummonsInFormation(Transform* formationRoot) const;
	void updateSummonTimer();

	// Fury
	bool m_furyRequested = false;
	bool m_furyActive = false;
	int m_furyIndex = 0;
	int m_furyCastsCompleted = 0;

	// Soul Cataclysm
	ComponentRef<Transform> m_soulCataclysmCenter;
	bool m_soulCataclysmTriggered = false;

	// Debug Draw
	bool m_hasSpiritCannonDebugLine = false;
	Vector3 m_spiritCannonDebugOrigin = Vector3::Zero;
	Vector3 m_spiritCannonDebugDirection = Vector3::Zero;
	float m_spiritCannonDebugWidth = 0.0f;

private:
	// Abilities
	std::vector<AelorinAbility> buildAbilityPool() const; // Normal ability pool
	std::vector<AelorinAbility> buildFuryAbilityPool() const; // Fury ability pool
	void removeLastUsedAbility(std::vector<AelorinAbility>& pool) const;

	bool canUseNova() const;
	bool canSummon() const;
	bool canTeleport() const;

	AelorinAbility chooseRandomAbility(const std::vector<AelorinAbility>& pool) const;

	bool isPlayerWithinDistance(float distance) const;
};