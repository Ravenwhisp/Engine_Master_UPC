#include "pch.h"
#include "AelorinSeekerSigilsState.h"
#include "AelorinAttackConfig.h"
#include "ProjectilePool.h"
#include "SeekerSigilProjectile.h"
#include "AelorinUI.h"

AelorinSeekerSigilsState::AelorinSeekerSigilsState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinSeekerSigilsState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinSeekerSigilsState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	// get scripts
	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_normalProjectilePool = m_controller->getSeekerSigilsProjectilePool();
	m_largeProjectilePool = m_controller->getSeekerSigilsLargeProjectilePool();
	m_aelorinUI = GameObjectAPI::findScript<AelorinUI>(parentGameObject);

	// reset members
	m_activeAbility = AelorinAbility::None;
	m_waveTimer = 0.0f;
	m_currentWave = 0;
	m_finalProjectileLaunched = false;
	m_completed = false;
	m_isFuryCast = false;

	if (!m_controller)
	{
		Debug::error("[AelorinSeekerSigilsState] AelorinBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[AelorinSeekerSigilsState] AnimationComponent not found.");
		return;
	}

	if (!m_normalProjectilePool)
	{
		Debug::error("[AelorinSeekerSigilsState] Normal ProjectilePool not found.");
		return;
	}

	if (!m_largeProjectilePool)
	{
		Debug::error("[AelorinSeekerSigilsState] Large ProjectilePool not found.");
		return;
	}

	if (!m_aelorinUI)
	{
		Debug::error("[AelorinSeekerSigilsState] AelorinUI not found.");
	}

	// consume ability
	m_activeAbility = m_controller->consumeRequestedAbility();
	if (m_activeAbility != AelorinAbility::SeekerSigils)
	{
		Debug::warn("[AelorinSeekerSigilsState] Unexpected requested ability!");
		return;
	}

	m_isFuryCast = m_controller->isFuryActive();
	if (m_isFuryCast)
	{
		m_controller->recordFuryCast();
	}

	Debug::log("[AelorinSeekerSigilsState] ENTER");
}

void AelorinSeekerSigilsState::OnStateUpdate()
{
	if (!m_controller || !m_animation || !m_normalProjectilePool || !m_largeProjectilePool || m_completed)
	{
		return;
	}

	if (m_controller->trySendPriorityInterrupt(m_animation))
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	m_waveTimer += Time::getDeltaTime();

	// If in fury mode -> delays + recovery is 0
	const float initialDelay = m_isFuryCast ? 0.0f : config->m_seekerSigilsInitialDelay;
	const float nextWaveTime = m_currentWave == 0 ? initialDelay : config->m_seekerSigilsWaveInterval;

	if (m_currentWave < config->m_seekerSigilsWaveCount)
	{
		if (m_waveTimer < nextWaveTime)
		{
			return;
		}

		launchCurrentWave();

		++m_currentWave;
		m_waveTimer = 0.0f;

		return;
	}

	if (m_controller->isPhase2() && !m_finalProjectileLaunched)
	{
		if (m_waveTimer < config->m_seekerSigilsPhase2FinalDelay)
		{
			return;
		}

		launchPhase2FinalProjectile();

		m_finalProjectileLaunched = true;
		m_waveTimer = 0.0f;
		
		return;
	}

	// Fury
	const float recoveryDuration = m_isFuryCast ? 0.0f : config->m_seekerSigilsRecoveryDuration;

	if (m_waveTimer < recoveryDuration)
	{
		return;
	}

	finishAbility();
}

void AelorinSeekerSigilsState::OnStateExit()
{
	if (m_aelorinUI)
	{
		m_aelorinUI->cancelSeekerSigils();
	}

	m_aelorinUI = nullptr;
	m_waveTimer = 0.0f;
	m_currentWave = 0;
	m_finalProjectileLaunched = false;
	m_completed = false;
	m_isFuryCast = false;

	Debug::log("[AelorinSeekerSigilsState] EXIT");
}

void AelorinSeekerSigilsState::launchCurrentWave()
{
	if (!m_controller)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();

	if (!config)
	{
		return;
	}

	const Vector3 lyrielPosition = m_controller->getLyrielPosition();
	const Vector3 deathPosition = m_controller->getDeathPosition();

	launchProjectileAt(m_normalProjectilePool, lyrielPosition, config->m_seekerSigilsRadius, config->m_seekerSigilsDamage);
	launchProjectileAt(m_normalProjectilePool, deathPosition, config->m_seekerSigilsRadius, config->m_seekerSigilsDamage);

	Debug::log("[AelorinSeekerSigilsState] Launched wave %d.", m_currentWave + 1);
}

void AelorinSeekerSigilsState::launchProjectileAt(ProjectilePool* projectilePool, const Vector3& targetPosition, float impactRadius, float damage)
{
	if (!projectilePool || !m_controller)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();

	if (!config)
	{
		return;
	}

	AelorinAttackExecutor* executor = m_controller->getAttackExecutor();

	if (!executor)
	{
		return;
	}

	ProjectileBase* pooledProjectile = projectilePool->acquireProjectile();

	if (!pooledProjectile)
	{
		Debug::warn("[AelorinSeekerSigilsState] No available Seeker Sigil Projectile.");
		return;
	}

	SeekerSigilProjectile* projectile = static_cast<SeekerSigilProjectile*>(pooledProjectile);
	Vector3 impactPosition = targetPosition;
	impactPosition.y = targetPosition.y;

	Vector3 spawnPosition = impactPosition;
	spawnPosition.y += config->m_seekerSigilsSpawnHeight;

	// UI
	if (!m_isFuryCast && m_aelorinUI && config->m_seekerSigilsFallSpeed > 0.0f)
	{
		const float telegraphDuration = config->m_seekerSigilsSpawnHeight / config->m_seekerSigilsFallSpeed;
		m_aelorinUI->showSeekerSigilsUI(impactPosition, impactRadius, telegraphDuration);
	}

	// Gameplay
	projectile->launch(spawnPosition, impactPosition, config->m_seekerSigilsFallSpeed, config->m_seekerSigilsProjectileLifetime, impactRadius, damage, executor);
}

void AelorinSeekerSigilsState::launchPhase2FinalProjectile()
{
	if (!m_controller)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();

	if (!config)
	{
		return;
	}

	const Vector3 lyrielPosition = m_controller->getLyrielPosition();
	const Vector3 deathPosition = m_controller->getDeathPosition();
	const Vector3 midpoint = (lyrielPosition + deathPosition) * 0.5f;

	launchProjectileAt(m_largeProjectilePool, midpoint, config->m_seekerSigilsPhase2FinalRadius, config->m_seekerSigilsPhase2FinalDamage);

	Debug::log("[AelorinSeekerSigilsState] Launched Phase 2 final projectile.");
}

void AelorinSeekerSigilsState::finishAbility()
{
	if (m_completed)
	{
		return;
	}

	m_completed = true;

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToIdle");
	if (!sent)
	{
		Debug::warn("[AelorinSeekerSigilsState] Failed to send ToIdle trigger.");
	}
}

IMPLEMENT_SCRIPT(AelorinSeekerSigilsState)