#include "pch.h"
#include "AelorinBossController.h"

#include "AelorinDetectionAggro.h"
#include "AelorinDamageable.h"
#include "AelorinAttackConfig.h"
#include "AelorinAttackExecutor.h"

#include "HealthDropSpawner.h"
#include "ProjectilePool.h"
#include "SeekerSigilProjectile.h"

#include "AelorinSummonSlot.h"
#include "Transform2D.h"

#include <vector>
#include <algorithm>
#include <cstdlib>

IMPLEMENT_SCRIPT_FIELDS_INHERITED(AelorinBossController, EnemyBaseController,
	SERIALIZED_BOOL(m_debugForcePhaseTransition, "DEBUG - Force Phase Transition"),
	SERIALIZED_ASSET_REF(m_attackConfig, "Attack Config", AssetType::DATA_CONTAINER),
	SERIALIZED_COMPONENT_REF(m_seekerSigilsProjectilePool, "Seeker Sigils Projectile Pool", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_seekerSigilsLargeProjectilePool, "Seeker Sigils Large Projectile Pool", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_risenSpiresPatternARoot, "Risen Spires Pattern A Root", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_risenSpiresPatternBRoot, "Risen Spires Pattern B Root", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_graspCenter, "Grasp Center", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_teleportAnchorsRoot, "Teleport Anchors Root", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_phase1SummonFormation, "Phase 1 Summon Formation", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_phase2SummonFormation, "Phase 2 Summon Formation", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_soulCataclysmCenter, "Soul Cataclysm Center", ComponentType::TRANSFORM),

	FIELD_GROUP_COLLAPSE("Shadow Mark Placement",
		SERIALIZED_COMPONENT_REF(m_shadowMarkPlacement,	"Shadow Mark Placement", ComponentType::TRANSFORM2D),
		SERIALIZED_FLOAT(m_shadowMarkPhase1Y, "Phase 1 Y", -1000.0f, 1000.0f, 1.0f),
		SERIALIZED_FLOAT(m_shadowMarkPhase1Scale, "Phase 1 Scale", 0.1f, 5.0f, 0.1f),
		SERIALIZED_FLOAT(m_shadowMarkPhase2Y, "Phase 2 Y", -1000.0f, 1000.0f, 1.0f),
		SERIALIZED_FLOAT(m_shadowMarkPhase2Scale, "Phase 2 Scale", 0.1f, 5.0f, 0.1f)
	)
)

AelorinBossController::AelorinBossController(GameObject* owner) : EnemyBaseController(owner)
{
}

void AelorinBossController::Start()
{
	EnemyBaseController::Start();

	m_aelorinDetectionAggro = GameObjectAPI::findScript<AelorinDetectionAggro>(getOwner());
	m_damageable = GameObjectAPI::findScript<AelorinDamageable>(getOwner());
	m_attackExecutor = GameObjectAPI::findScript<AelorinAttackExecutor>(getOwner());

	Transform* phase1Model = TransformAPI::findChildByName(getOwner()->GetTransform(), "Phase1");
	if (!phase1Model)
	{
		Debug::error("[AelorinBossController] Phase 1 Model not found!");
		return;
	}	

	Transform* phase2Model = TransformAPI::findChildByName(getOwner()->GetTransform(), "Phase2");
	if (!phase2Model)
	{
		Debug::error("[AelorinBossController] Phase 2 Model not found!");
		return;
	}

	m_phase1GameObject = ComponentAPI::getOwner(phase1Model);
	m_phase2GameObject = ComponentAPI::getOwner(phase2Model);

	if (!m_aelorinDetectionAggro)
	{
		Debug::error("[AelorinBossController] AelorinDetectionAggro script not found!");
	}

	if (!m_damageable)
	{
		Debug::error("[AelorinBossController] AelorinDamageable script not found!");
	}

	if (!m_attackExecutor)
	{
		Debug::error("[AelorinBossController] AelorinAttackExecutor script not found!");
	}

	if (!m_phase1GameObject)
	{
		Debug::error("[AelorinBossController] Phase 1 Game Object not found!");
	}

	if (!m_phase2GameObject)
	{
		Debug::error("[AelorinBossController] Phase 2 Game Object not found!");
	}

	Transform* normalPoolTransform = m_seekerSigilsProjectilePool.getReferencedComponent();
	if (normalPoolTransform)
	{
		m_seekerSigilsProjectilePoolScript = GameObjectAPI::findScript<ProjectilePool>(normalPoolTransform->getOwner());
	}

	Transform* largePoolTransform = m_seekerSigilsLargeProjectilePool.getReferencedComponent();
	if (largePoolTransform)
	{
		m_seekerSigilsLargeProjectilePoolScript = GameObjectAPI::findScript<ProjectilePool>(largePoolTransform->getOwner());
	}

	if (!m_seekerSigilsProjectilePoolScript)
	{
		Debug::error("[AelorinBossController] Normal Seeker Sigils ProjectilePool not found.");
	}

	if (!m_seekerSigilsLargeProjectilePoolScript)
	{
		Debug::error("[AelorinBossController] Large Seeker Sigils ProjectilePool not found.");
	}

	applyShadowMarkPlacement();
}

//void AelorinBossController::drawGizmo()
//{
//
//}

void AelorinBossController::Update()
{
	if (m_debugForcePhaseTransition)
	{
		if (!isPhase2())
		{
			requestPhaseTransition();
			Debug::log("[AelorinBossController] DEBUG: Forced Phase Transition.");
		}
	}

	updateEncounter();
	updateTeleportCooldown();
	updateSummonTimer();	
}

Transform* AelorinBossController::getLyrielTransform() const
{
	if (!m_aelorinDetectionAggro)
	{
		return nullptr;
	}

	return m_aelorinDetectionAggro->getLyrielTransform();
}

Transform* AelorinBossController::getDeathTransform() const
{
	if (!m_aelorinDetectionAggro)
	{
		return nullptr;
	}

	return m_aelorinDetectionAggro->getDeathTransform();
}

Vector3 AelorinBossController::getLyrielPosition() const
{
	if (!m_aelorinDetectionAggro)
	{
		return Vector3::Zero;
	}

	return m_aelorinDetectionAggro->getLyrielPosition();
}

Vector3 AelorinBossController::getDeathPosition() const
{
	if (!m_aelorinDetectionAggro)
	{
		return Vector3::Zero;
	}

	return m_aelorinDetectionAggro->getDeathPosition();
}

float AelorinBossController::getClosestPlayerDistance() const
{
	if (!m_aelorinDetectionAggro)
	{
		return FLT_MAX;
	}

	const float minDistance = (std::min)(m_aelorinDetectionAggro->getDistanceToLyriel(), m_aelorinDetectionAggro->getDistanceToDeath());
	
	return minDistance;
}

AelorinAbility AelorinBossController::chooseNextAbility()
{
	if (canTeleport())
	{
		return AelorinAbility::Teleport;
	}

	std::vector<AelorinAbility> pool = buildAbilityPool();
	removeLastUsedAbility(pool);
	
	return chooseRandomAbility(pool);
}

AelorinAbility AelorinBossController::chooseNextFuryAbility()
{
	std::vector<AelorinAbility> pool = buildFuryAbilityPool();
	removeLastUsedAbility(pool);

	return chooseRandomAbility(pool);
}

AelorinAbility AelorinBossController::consumeRequestedAbility()
{
	const AelorinAbility ability = m_requestedAbility;

	if (ability != AelorinAbility::None)
	{
		m_lastUsedAbility = ability;
	}

	clearRequestedAbility();

	return ability;
}

bool AelorinBossController::requestAbility(AelorinAbility ability)
{
	if (ability == AelorinAbility::None || hasRequestedAbility())
	{
		return false;
	}

	m_requestedAbility = ability;
	m_abilityTriggerSent = false;

	return true;
}

bool AelorinBossController::trySendRequestedAbilityTrigger(AnimationComponent* animation)
{
	if (!hasRequestedAbility() || m_abilityTriggerSent || !animation)
	{
		return false;
	}

	const char* triggerName = nullptr;

	switch (m_requestedAbility)
	{
	case AelorinAbility::SeekerSigils:
		triggerName = "ToSeekerSigils";
		break;

	case AelorinAbility::Nova:
		triggerName = "ToNova";
		break;

	case AelorinAbility::RisenSpires:
		triggerName = "ToRisenSpires";
		break;

	case AelorinAbility::SpiritCannon:
		triggerName = "ToSpiritCannon";
		break;

	case AelorinAbility::GraspOfTheDead:
		triggerName = "ToGraspOfTheDead";
		break;

	case AelorinAbility::Summon:
		triggerName = "ToSummon";
		break;

	case AelorinAbility::Teleport:
		triggerName = "ToTeleport";
		break;

	case AelorinAbility::None:
	default:
		return false;
	}

	const bool sent = AnimationAPI::sendTrigger(animation, triggerName);

	if (!sent)
	{
		return false;
	}

	m_abilityTriggerSent = true;
	Debug::log("[AelorinBossController] Ability trigger sent: %s", triggerName);

	return true;
}

void AelorinBossController::clearRequestedAbility()
{
	m_requestedAbility = AelorinAbility::None;
	m_abilityTriggerSent = false;
}

void AelorinBossController::updateEncounter()
{
	if (!m_hasStartedEncounter && m_aelorinDetectionAggro)
	{
		if (m_aelorinDetectionAggro->startEncounter())
		{
			m_hasStartedEncounter = true;
		}
	}
}

float AelorinBossController::getDecisionTime() const
{
	const AelorinAttackConfig* config = m_attackConfig.get();

	if (!config)
	{
		return isPhase2() ? 1.0f : 2.0f;
	}

	return isPhase2() ? config->m_phase2DecisionTime : config->m_phase1DecisionTime;
}

void AelorinBossController::setPhase(Phase phase)
{
	m_phase = phase;
}

void AelorinBossController::requestPhaseTransition()
{
	if (isPhase2() || m_phaseTransitionRequested)
	{
		return;
	}

	m_phaseTransitionRequested = true;

	Debug::log("[AelorinBossController] Phase transition requested.");
}

void AelorinBossController::markPhaseTransitionTriggered()
{
	m_phaseTransitionTriggered = true;
}

void AelorinBossController::beginPhase2()
{
	if (isPhase2())
	{
		return;
	}

	if (!m_phase1GameObject || !m_phase2GameObject)
	{
		return;
	}

	GameObjectAPI::setActive(m_phase1GameObject, false);
	GameObjectAPI::setActive(m_phase2GameObject, true);

	setPhase(Phase::Phase2);

	applyShadowMarkPlacement();

	m_phaseTransitionRequested = false;
	m_phaseTransitionTriggered = false;

	if (m_damageable)
	{
		m_damageable->beginPhase2();
	}

	Debug::log("[AelorinBossController] Phase 2 started.");
}

bool AelorinBossController::trySendPhaseTransitionTrigger(AnimationComponent* animation)
{
	if (!m_phaseTransitionRequested)
	{
		return false;
	}

	if (m_phaseTransitionTriggered)
	{
		return false;
	}

	if (!animation)
	{
		return false;
	}

	const bool sent = AnimationAPI::sendTrigger(animation, "ToPhaseTransition");

	if (!sent)
	{
		return false;
	}

	markPhaseTransitionTriggered();

	Debug::log("[AelorinBossController] ToPhaseTransition trigger sent.");

	return true;
}

void AelorinBossController::requestThresholdStagger()
{
	if (m_thresholdStaggerRequested)
	{
		return;
	}

	m_thresholdStaggerRequested = true;
	m_thresholdStaggerTriggered = false;

	Debug::log("[AelorinBossController] Threshold stagger requested.");
}

bool AelorinBossController::trySendThresholdStaggerTrigger(AnimationComponent* animation)
{
	if (!m_thresholdStaggerRequested)
	{
		return false;
	}

	if (m_thresholdStaggerTriggered)
	{
		return false;
	}

	if (!animation)
	{
		return false;
	}

	const bool sent = AnimationAPI::sendTrigger(animation, "ToThresholdStagger");

	if (!sent)
	{
		return false;
	}

	m_thresholdStaggerTriggered = true;

	Debug::log("[AelorinBossController] ToThresholdStagger trigger sent.");

	return true;
}

void AelorinBossController::completeThresholdStagger()
{
	m_thresholdStaggerRequested = false;
	m_thresholdStaggerTriggered = false;

	Debug::log("[AelorinBossController] Threshold stagger completed.");
}

float AelorinBossController::getThresholdStaggerDuration() const
{
	const AelorinAttackConfig* config = m_attackConfig.get();

	if (!config)
	{
		return 2.0f;
	}

	return config->m_thresholdStaggerDuration;
}

void AelorinBossController::setSpiritCannonDebugLine(const Vector3& origin, const Vector3& direction, float width)
{
	m_spiritCannonDebugOrigin = origin;
	m_spiritCannonDebugDirection = direction;
	m_spiritCannonDebugWidth = width;
	m_hasSpiritCannonDebugLine = true;
}

void AelorinBossController::clearSpiritCannonDebugLine()
{
	m_spiritCannonDebugOrigin = Vector3::Zero;
	m_spiritCannonDebugDirection = Vector3::Zero;
	m_spiritCannonDebugWidth = 0.0f;
	m_hasSpiritCannonDebugLine = false;
}

void AelorinBossController::prepareForcedNovaAt(const Vector3& center)
{
	m_novaCenterOverride = center;
	m_hasNovaCenterOverride = true;
	m_requestedAbility = AelorinAbility::Nova;
}

Vector3 AelorinBossController::consumeNovaCenterOverride()
{
	const Vector3 center = m_novaCenterOverride;
	m_novaCenterOverride = Vector3::Zero;
	m_hasNovaCenterOverride = false;
	return center;
}

void AelorinBossController::startTeleportCooldown()
{
	const AelorinAttackConfig* config = getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	m_teleportCooldownRemaining = config->m_teleportCooldownDuration;
}

void AelorinBossController::updateTeleportCooldown()
{
	if (m_teleportCooldownRemaining <= 0.0f)
	{
		return;
	}

	m_teleportCooldownRemaining -= Time::getDeltaTime();
	if (m_teleportCooldownRemaining < 0.0f)
	{
		m_teleportCooldownRemaining = 0.0f;
	}
}

int AelorinBossController::getLivingSummonCount() const
{
	const int phase1Living = countLivingSummonsInFormation(getPhase1SummonFormation());
	const int phase2Living = countLivingSummonsInFormation(getPhase2SummonFormation());

	return phase1Living + phase2Living;
}

int AelorinBossController::getCurrentSummonCap() const
{
	Transform* formationRoot = isPhase2() ? getPhase2SummonFormation() : getPhase1SummonFormation();
	if (!formationRoot)
	{
		return 0;
	}

	return TransformAPI::getChildCount(formationRoot);
}

void AelorinBossController::startSummonTimer()
{
	const AelorinAttackConfig* config =	getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	m_summonTimerRemaining = isPhase2()	? config->m_phase2SummonInterval : config->m_phase1SummonInterval;
}

void AelorinBossController::requestFury()
{
	if (!isPhase2())
	{
		return;
	}

	if (m_furyIndex >= 2)
	{
		return;
	}

	if (m_furyRequested || m_furyActive)
	{
		return;
	}

	m_furyRequested = true;

	Debug::log("[AelorinBossController] Fury %d requested", m_furyIndex + 1);
}

void AelorinBossController::beginFury()
{
	if (!m_furyRequested || m_furyActive)
	{
		return;
	}

	m_furyRequested = false;
	m_furyActive = true;
	m_furyCastsCompleted = 0;
	m_soulCataclysmTriggered = false;

	Debug::log("[AelorinBossController] Fury %d started", m_furyIndex + 1);
}

void AelorinBossController::recordFuryCast()
{
	if (!m_furyActive)
	{
		return;
	}

	++m_furyCastsCompleted;
}

void AelorinBossController::finishFury()
{
	if (!m_furyActive)
	{
		return;
	}

	m_furyRequested = false;
	m_furyActive = false;
	m_furyCastsCompleted = 0;
	m_soulCataclysmTriggered = false;

	clearRequestedAbility();
	m_lastUsedAbility = AelorinAbility::None;

	++m_furyIndex;
}

bool AelorinBossController::isFuryBarrageComplete() const
{
	if (!m_furyActive)
	{
		return false;
	}

	return m_furyCastsCompleted >= getFuryCastTarget();
}

int AelorinBossController::getFuryCastTarget() const
{
	const AelorinAttackConfig* config = m_attackConfig.get();
	if (!config)
	{
		return m_furyIndex == 0 ? 6 : 10;
	}

	return m_furyIndex == 0 ? config->m_firstFuryCastCount : config->m_secondFuryCastCount;
}

Transform* AelorinBossController::getTeleportCrowdingPlayer() const
{
	if (!m_aelorinDetectionAggro)
	{
		return nullptr;
	}

	const AelorinAttackConfig* config = getAelorinAttackConfig();
	if (!config)
	{
		return nullptr;
	}

	Transform* lyriel = m_aelorinDetectionAggro->getLyrielTransform();
	Transform* death = m_aelorinDetectionAggro->getDeathTransform();

	const float triggerDistance = config->m_teleportTriggerDistance;
	const float lyrielDistance = m_aelorinDetectionAggro->getDistanceToLyriel();
	const float deathDistance = m_aelorinDetectionAggro->getDistanceToDeath();

	const bool lyrielCrowding = lyriel && lyrielDistance <= triggerDistance;
	const bool deathCrowding = death && deathDistance <= triggerDistance;

	if (lyrielCrowding && !deathCrowding)
	{
		return lyriel;
	}

	if (!lyrielCrowding && deathCrowding)
	{
		return death;
	}

	if (lyrielCrowding && deathCrowding)
	{
		return lyrielDistance <= deathDistance ? lyriel : death;
	}

	return nullptr;
}

Transform* AelorinBossController::chooseTeleportAnchor(Transform* crowdingPlayer) const
{
	if (!crowdingPlayer)
	{
		return nullptr;
	}

	Transform* anchorsRoot = getTeleportAnchorsRoot();
	if (!anchorsRoot)
	{
		return nullptr;
	}

	const AelorinAttackConfig* config = getAelorinAttackConfig();
	if (!config)
	{
		return nullptr;
	}

	const Vector3 playerPosition = TransformAPI::getGlobalPosition(crowdingPlayer);
	const float clearDistance = config->m_teleportTriggerDistance;
	const float clearDistanceSquared = clearDistance * clearDistance;

	std::vector<Transform*> suitableAnchors;

	const int childCount = TransformAPI::getChildCount(anchorsRoot);

	for (int i = 0; i < childCount; ++i)
	{
		Transform* anchor = TransformAPI::getChild(anchorsRoot, i);
		if (!anchor)
		{
			continue;
		}

		const Vector3 anchorPosition = TransformAPI::getGlobalPosition(anchor);
		Vector3 difference = anchorPosition - playerPosition;
		difference.y = 0.0f;

		if (difference.LengthSquared() < clearDistanceSquared)
		{
			continue;
		}

		suitableAnchors.push_back(anchor);
	}

	if (suitableAnchors.empty())
	{
		Debug::warn("[AelorinBossController] No teleport anchor clears the crowding player");
		return nullptr;
	}

	const int randomIndex = std::rand() % static_cast<int>(suitableAnchors.size());

	return suitableAnchors[randomIndex];
}

bool AelorinBossController::trySendSoulCataclysmTrigger(AnimationComponent* animation)
{
	if (!m_furyActive)
	{
		return false;
	}

	if (!isFuryBarrageComplete())
	{
		return false;
	}

	if (m_soulCataclysmTriggered)
	{
		return false;
	}

	if (!animation)
	{
		return false;
	}

	const bool sent = AnimationAPI::sendTrigger(animation, "ToSoulCataclysm");
	if (!sent)
	{
		return false;
	}

	m_soulCataclysmTriggered = true;
	Debug::log("[AelorinBossController] Soul Cataclysm triggered");

	return true;
}

void AelorinBossController::spawnHealthDrops()
{
	const AelorinAttackConfig* config = m_attackConfig.get();

	if (!config)
	{
		Debug::warn("[AelorinBossController] Cannot spawn health drops: AelorinAttackConfig is missing.");
		return;
	}

	if (!config->m_healthPickupPrefab.m_id.isValid())
	{
		Debug::warn("[AelorinBossController] Cannot spawn health drops: health pickup prefab is invalid.");
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		Debug::warn("[AelorinBossController] Cannot spawn health drops: owner transform is missing.");
		return;
	}

	const Vector3 position = TransformAPI::getGlobalPosition(ownerTransform);

	for (int i = 0; i < config->m_healthDropQuantity; ++i)
	{
		HealthDropSpawner::drop(
			config->m_healthPickupPrefab.m_id,
			position,
			config->m_healingAmount,
			config->m_dropRadius,
			config->m_dropHeight
		);
	}
}

bool AelorinBossController::trySendPriorityInterrupt(AnimationComponent* animation)
{
	if (!animation)
	{
		return false;
	}

	// Highest priority
	if (isPhase2() && trySendDeathTrigger(animation))
	{
		return true;
	}

	// Phase 1 only
	if (trySendPhaseTransitionTrigger(animation))
	{
		return true;
	}

	// Standard / Fury threshold stagger
	if (trySendThresholdStaggerTrigger(animation))
	{
		return true;
	}

	return false;
}

// These two will not be needed
Transform* AelorinBossController::acquireCurrentTarget()
{
	return nullptr;
}

bool AelorinBossController::isTargetDowned(Transform* target) const
{
	return false;
}
// -----------------------------

int AelorinBossController::countLivingSummonsInFormation(Transform* formationRoot) const
{
	if (!formationRoot)
	{
		return 0;
	}

	int livingCount = 0;

	const int slotCount = TransformAPI::getChildCount(formationRoot);

	for (int i = 0; i < slotCount; ++i)
	{
		Transform* slotTransform = TransformAPI::getChild(formationRoot, i);
		if (!slotTransform)
		{
			continue;
		}

		GameObject* slotObject = ComponentAPI::getOwner(slotTransform);
		if (!slotObject)
		{
			continue;
		}

		AelorinSummonSlot* slot = GameObjectAPI::findScript<AelorinSummonSlot>(slotObject);
		if (!slot)
		{
			continue;
		}

		if (slot->hasLivingEnemy())
		{
			++livingCount;
		}
	}

	return livingCount;
}

void AelorinBossController::updateSummonTimer()
{
	if (m_summonTimerRemaining <= 0.0f)
	{
		return;
	}

	m_summonTimerRemaining -= Time::getDeltaTime();

	if (m_summonTimerRemaining < 0.0f)
	{
		m_summonTimerRemaining = 0.0f;
	}
}

void AelorinBossController::applyShadowMarkPlacement()
{
	Transform2D* placement = m_shadowMarkPlacement.getReferencedComponent();
	if (!placement)
	{
		return;
	}

	GameObject* placementObject = ComponentAPI::getOwner(placement);
	if (!placementObject)
	{
		return;
	}

	Transform* placementTransform = GameObjectAPI::getTransform(placementObject);
	Transform* bossTransform = GameObjectAPI::getTransform(getOwner());
	if (!placementTransform || !bossTransform)
	{
		return;
	}

	const Vector3 bossPosition = TransformAPI::getGlobalPosition(bossTransform);

	if (isPhase2())
	{
		const Vector3 offset(0.0f, m_shadowMarkPhase2Y,	0.0f);
		TransformAPI::setGlobalPosition(placementTransform,	bossPosition + offset);
		Transform2DAPI::setScale(placement,	Vector2(m_shadowMarkPhase2Scale, m_shadowMarkPhase2Scale));

		return;
	}

	const Vector3 offset(0.0f, m_shadowMarkPhase1Y, 0.0f);
	TransformAPI::setGlobalPosition(placementTransform,	bossPosition + offset);
	Transform2DAPI::setScale(placement, Vector2(m_shadowMarkPhase1Scale, m_shadowMarkPhase1Scale));
}

std::vector<AelorinAbility> AelorinBossController::buildAbilityPool() const
{
	std::vector<AelorinAbility> pool
	{
		AelorinAbility::SeekerSigils,
		AelorinAbility::RisenSpires,
		AelorinAbility::SpiritCannon
	};

	if (canUseNova())
	{
		pool.push_back(AelorinAbility::Nova);
	}

	if (canSummon())
	{
		pool.push_back(AelorinAbility::Summon);
	}

	if (isPhase2())
	{
		pool.push_back(AelorinAbility::GraspOfTheDead);
	}

	return pool;
}

std::vector<AelorinAbility> AelorinBossController::buildFuryAbilityPool() const
{
	std::vector<AelorinAbility> pool
	{
		AelorinAbility::SeekerSigils,
		AelorinAbility::RisenSpires,
		AelorinAbility::SpiritCannon,
		AelorinAbility::GraspOfTheDead
	};

	return pool;
}

void AelorinBossController::removeLastUsedAbility(std::vector<AelorinAbility>& pool) const
{
	if (m_lastUsedAbility == AelorinAbility::None || pool.size() <= 1)
	{
		return;
	}

	pool.erase(std::remove(pool.begin(), pool.end(), m_lastUsedAbility), pool.end());
}

bool AelorinBossController::canUseNova() const
{
	const AelorinAttackConfig* config = getAelorinAttackConfig();

	if (!config)
	{
		return false;
	}

	return isPlayerWithinDistance(config->m_novaTriggerDistance);
}

bool AelorinBossController::canSummon() const
{
	if (!isSummonReady())
	{
		return false;
	}

	return getLivingSummonCount() < getCurrentSummonCap();
}

bool AelorinBossController::canTeleport() const
{
	if (!isTeleportReady())
	{
		return false;
	}

	return getTeleportCrowdingPlayer() != nullptr;
}

AelorinAbility AelorinBossController::chooseRandomAbility(const std::vector<AelorinAbility>& pool) const
{
	if (pool.empty())
	{
		return AelorinAbility::None;
	}

	const std::size_t index = static_cast<std::size_t>(std::rand()) % pool.size();

	return pool[index];
}

bool AelorinBossController::isPlayerWithinDistance(float distance) const
{
	return getClosestPlayerDistance() <= distance;
}

IMPLEMENT_SCRIPT(AelorinBossController)