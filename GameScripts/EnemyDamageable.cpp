#include "pch.h"
#include "EnemyDamageable.h"

#include "EnemyDetectionAggro.h"

EnemyDamageable::EnemyDamageable(GameObject* owner)
	: Damageable(owner)
{
}

void EnemyDamageable::Start()
{
	Damageable::Start();

	m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(m_owner);

	if (!m_enemyDetectionAggro)
	{
		Debug::warn("EnemyDetectionAggro Script is missing from %s", GameObjectAPI::getName(m_owner));
	}

}

void EnemyDamageable::onDamaged(float amount)
{
	Damageable::onDamaged(amount);
	
	if (!m_enemyDetectionAggro)
	{
		return;
	}

	if (!m_damageSource)
	{
		return;
	}

	m_enemyDetectionAggro->notifyPlayerAttackedEnemy(m_damageSource);
}

void EnemyDamageable::takeDamageEnemy(float amount, Transform* playerTransform)
{
	if (playerTransform)
	{
		m_damageSource = playerTransform;
	}

	Damageable::takeDamage(amount);

	m_damageSource = nullptr;
}

IMPLEMENT_SCRIPT(EnemyDamageable)