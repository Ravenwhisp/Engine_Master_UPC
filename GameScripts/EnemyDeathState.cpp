#include "pch.h"
#include "EnemyDeathState.h"
#include "HealthPickup.h"
#include "HealthDropSpawner.h"
#include "EnemySound.h"
#include "EnemyDamageable.h"

IMPLEMENT_SCRIPT_FIELDS(EnemyDeathState,
	SERIALIZED_FLOAT(m_dissolveDelay, "Dissolve Delay", 0.0f, 30.0f, 0.1f),
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

	m_enemyDamageable = GameObjectAPI::findScript<EnemyDamageable>(getOwner());
	m_dissolveStarted = false;
	m_destroyQueued = false;

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

	startDestroyCountdown(m_dissolveDelay);
}

void EnemyDeathState::OnStateUpdate()
{
	if (m_destroyQueued)
	{
		m_destroyQueued = false;
		GameObjectAPI::removeGameObject(getOwner());
		return;
	}

	if (!m_waitingToDestroy || m_deathFinished || m_deathPaused)
	{
		return;
	}

	m_deathTimer -= Time::getDeltaTime();

	if (!m_dissolveStarted)
	{
		m_deathTimer -= Time::getDeltaTime();

		if (m_deathTimer <= 0.0f)
		{
			if (m_enemyDamageable)
			{
				m_enemyDamageable->startDissolve();
				m_dissolveStarted = true;
			}
			else
			{
				m_deathFinished = true;
				m_waitingToDestroy = false;
				onDeathFinished();
			}
		}

		return;
	}

	if (m_enemyDamageable->isDissolveFinished())
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
	m_destroyQueued = true;
}

void EnemyDeathState::dropRewards()
{
    if (!m_healthPrefab.m_id.isValid())
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
        HealthDropSpawner::drop(m_healthPrefab.m_id,
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

void EnemyDeathState::resumeDeathCountdown()
{
	m_deathPaused = false;
}

void EnemyDeathState::finalizeDeathNow()
{
	m_deathPaused = false;
	if (m_shouldDropHealth)
	{
		dropRewards();
	}
	startDestroyCountdown(m_dissolveDelay);
}

void EnemyDeathState::abortDeathForRevival()
{
	m_deathPaused = false;
	m_waitingToDestroy = false;
	m_deathFinished = false;
	m_deathTimer = 0.0f;
}

IMPLEMENT_SCRIPT(EnemyDeathState)
