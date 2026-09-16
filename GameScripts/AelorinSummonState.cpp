#include "pch.h"
#include "AelorinSummonState.h"

#include "AelorinAttackConfig.h"
#include "AelorinSummonSlot.h"

#include <algorithm>

AelorinSummonState::AelorinSummonState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinSummonState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinSummonState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	// get scripts
	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	// reset members
	m_activeAbility = AelorinAbility::None;
	m_stateTimer = 0.0f;
	m_recoveryTimer = 0.0f;
	m_summonExecuted = false;
	m_completed = false;

	if (!m_controller)
	{
		Debug::error("[AelorinSummonState] AelorinBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[AelorinSummonState] AnimationComponent not found.");
		return;
	}

	// consume ability
	m_activeAbility = m_controller->consumeRequestedAbility();
	if (m_activeAbility != AelorinAbility::Summon)
	{
		Debug::warn("[AelorinSummonState] Unexpected requested ability!");
		return;
	}

	Debug::log("[AelorinTeleportState] ENTER");
}

void AelorinSummonState::OnStateUpdate()
{
	if (!m_controller || !m_animation || m_completed)
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

	if (!m_summonExecuted)
	{
		m_stateTimer += Time::getDeltaTime();

		if (m_stateTimer < config->m_summonCastDuration)
		{
			return;
		}

		executeSummon();

		m_summonExecuted = true;
		return;
	}

	m_recoveryTimer += Time::getDeltaTime();

	if (m_recoveryTimer < config->m_summonRecoveryDuration)
	{
		return;
	}

	finishAbility();
}

void AelorinSummonState::OnStateExit()
{
	m_stateTimer = 0.0f;
	m_recoveryTimer = 0.0f;
	m_summonExecuted = false;
	m_completed = false;
	Debug::log("[AelorinSummonState] EXIT");
}

void AelorinSummonState::executeSummon()
{
	if (!m_controller)
	{
		return;
	}

	const int livingCount =	m_controller->getLivingSummonCount();
	const int cap =	m_controller->getCurrentSummonCap();
	const int missing =	(std::max)(0, cap - livingCount);

	if (missing <= 0)
	{
		Debug::log("[AelorinSummonState] No enemies need to be summoned.");
		return;
	}

	Transform* formationRoot = m_controller->isPhase2()	? m_controller->getPhase2SummonFormation() : m_controller->getPhase1SummonFormation();
	if (!formationRoot)
	{
		Debug::warn("[AelorinSummonState] Summon formation not assigned.");
		return;
	}

	const int spawnedCount = fillFormation(formationRoot, missing);

	if (spawnedCount > 0)
	{
		m_controller->startSummonTimer();
	}

	Debug::log("[AelorinSummonState] Summoned %d / %d missing enemies.", spawnedCount, missing);
}

int AelorinSummonState::fillFormation(Transform* formationRoot, int maxToSpawn)
{
	if (!formationRoot || maxToSpawn <= 0)
	{
		return 0;
	}

	int spawnedCount = 0;
	const int slotCount = TransformAPI::getChildCount(formationRoot);

	for (int i = 0;	i < slotCount && spawnedCount < maxToSpawn;	++i)
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
			continue;
		}

		GameObject* enemy =	slot->spawnEnemy();
		if (!enemy)
		{
			continue;
		}

		++spawnedCount;
	}

	return spawnedCount;
}

void AelorinSummonState::finishAbility()
{
	if (m_completed)
	{
		return;
	}

	m_completed = true;

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToIdle");
	if (!sent)
	{
		Debug::warn("[AelorinSummonState] Failed to send ToIdle trigger.");
	}
}

IMPLEMENT_SCRIPT(AelorinSummonState)