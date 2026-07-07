#include "pch.h"
#include "EnemyDeathState.h"
#include "HealthPickup.h"
#include "HealthDropSpawner.h"
#include "EnemySound.h"

IMPLEMENT_SCRIPT_FIELDS(EnemyDeathState,
	SERIALIZED_FLOAT(m_destroyDelay, "Destroy Delay", 0.0f, 30.0f, 0.1f),
	SERIALIZED_BOOL(m_shouldDropHealth, "Should Drop Health"),
	SERIALIZED_ASSET_REF(m_healthPrefab, "Health Prefab", AssetType::PREFAB),
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

	EnemySound* enemySound = GameObjectAPI::findScript<EnemySound>(getOwner());
	if (enemySound)
	{
		enemySound->stopAllLoops();   // kill charge loop / footsteps before the death sting
		enemySound->playDeath();
	}

	onDeathStarted();

	if (m_shouldDropHealth)
	{
		dropRewards();
	}

	startDestroyCountdown(m_destroyDelay);
}

void EnemyDeathState::OnStateUpdate()
{
	if (!m_waitingToDestroy || m_deathFinished || m_deathPaused)
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
        if (!m_healthPrefab.m_ref.isValid())
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
        HealthDropSpawner::drop(m_healthPrefabPath.c_str(),
                                spawnPosition,
                                m_dropHealAmount,
                                m_dropRadius,
                                m_dropHeight);
    }
}

void EnemyDeathState::pauseDeathCountdown()
{
	m_deathPaused = true;
}


		float distance = (static_cast<float>(rand()) / RAND_MAX) * m_dropRadius;


		Vector3 offset;
		offset.x = std::cos(angle) * distance;
		offset.z = std::sin(angle) * distance;
		offset.y = 0.0f;

		Vector3 finalPos = spawnPosition + offset;
		Vector3 arcOrigin = Vector3(spawnPosition.x, spawnPosition.y + m_dropHeight, spawnPosition.z);

        // Instantiate at the arc origin (enemy center) so the pickup is never
        // visible at the floor position before Start() runs.
        GameObject* pickup = GameObjectAPI::instantiatePrefab(m_healthPrefab.m_ref, arcOrigin, Vector3::Zero);

		if (pickup == nullptr)
		{
			continue;
		}

void EnemyDeathState::finalizeDeathNow()
{
	m_deathPaused = false;
	if (m_shouldDropHealth)
	{
		dropRewards();
	}
	startDestroyCountdown(m_destroyDelay);
}

void EnemyDeathState::abortDeathForRevival()
{
	m_deathPaused = false;
	m_waitingToDestroy = false;
	m_deathFinished = false;
	m_deathTimer = 0.0f;
}

IMPLEMENT_SCRIPT(EnemyDeathState)