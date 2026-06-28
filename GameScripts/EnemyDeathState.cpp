#include "pch.h"
#include "EnemyDeathState.h"
#include "HealthPickup.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(EnemyDeathState,
	SERIALIZED_FLOAT(m_destroyDelay, "Destroy Delay", 0.0f, 30.0f, 0.1f),
	SERIALIZED_BOOL(m_shouldDropHealth, "Should Drop Health"),
	SERIALIZED_STRING(m_healthPrefabPath, "Health Prefab Path"),
	SERIALIZED_INT(m_healthDropQuantity, "Health Drop Quantity"),
	SERIALIZED_FLOAT(m_dropHealAmount, "Drop Heal Amount", 0.0f, 100.0f, 1.0f),
	SERIALIZED_FLOAT(m_dropRadius, "Drop Radius", 0.0f, 5.0f, 0.1f),
	SERIALIZED_FLOAT(m_dropHeight, "Drop Height", 0.0f, 5.0f, 0.1f)
)

EnemyDeathState::EnemyDeathState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void EnemyDeathState::OnStateEnter()
{
	Debug::log("[EnemyDeathState] ENTER");
	onDeathStarted();

	if (m_shouldDropHealth)
	{
		dropRewards();
	}

	startDestroyCountdown(m_destroyDelay);
}

void EnemyDeathState::OnStateUpdate()
{
	if (!m_waitingToDestroy || m_deathFinished)
	{
		return;
	}

	m_deathTimer -= Time::getDeltaTime();

	if (m_deathTimer <= 0.0f)
	{
		m_deathFinished = true;
		m_waitingToDestroy = false;

		onDeathFinished();
	}
}

void EnemyDeathState::OnStateExit()
{
}

void EnemyDeathState::onDeathStarted()
{
	// on death enemy specific trigger logic here
	// example: unlock a door
}

void EnemyDeathState::onDeathFinished()
{
	destroyEnemyNow();
}

void EnemyDeathState::startDestroyCountdown(float delay)
{
	m_waitingToDestroy = true;
	m_deathTimer = delay;
}

void EnemyDeathState::destroyEnemyNow()
{
	m_waitingToDestroy = false;
	GameObjectAPI::removeGameObject(getOwner());
}

void EnemyDeathState::dropRewards()
{
	if (m_healthPrefabPath.empty())
	{
		return;
	}

	const Transform* myTransform = GameObjectAPI::getTransform(getOwner());
	if (myTransform == nullptr)
	{
		return;
	}

	const Vector3 spawnPosition = TransformAPI::getGlobalPosition(myTransform);

	for (int i = 0; i < m_healthDropQuantity; ++i)
	{

		float angle = (static_cast<float>(rand()) / RAND_MAX) * 6.283185f;


		float distance = (static_cast<float>(rand()) / RAND_MAX) * m_dropRadius;


		Vector3 offset;
		offset.x = std::cos(angle) * distance;
		offset.z = std::sin(angle) * distance;
		offset.y = 0.0f;

		Vector3 finalPos = spawnPosition + offset;
		Vector3 arcOrigin = Vector3(spawnPosition.x, spawnPosition.y + m_dropHeight, spawnPosition.z);

		// Instantiate at the arc origin (enemy center) so the pickup is never
		// visible at the floor position before Start() runs.
		GameObject* pickup = GameObjectAPI::instantiatePrefab(m_healthPrefabPath.c_str(), arcOrigin, Vector3::Zero);

		if (pickup == nullptr)
		{
			continue;
		}

        Script* script = GameObjectAPI::getScript(pickup, "HealthPickup");
        if (script != nullptr)
        {
            HealthPickup* healthPickup = static_cast<HealthPickup*>(script);
            healthPickup->m_healAmount = m_dropHealAmount;
            healthPickup->m_landingPosition = finalPos;
            healthPickup->m_hasCustomSpawnFrom = true;
        }
    }
}

IMPLEMENT_SCRIPT(EnemyDeathState)