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

	if (!m_healthBarSlider || !m_healthBar2Slider)
	{
		Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
		Transform* healthBarTransform = TransformAPI::findChildByName(ownerTransform, "Health Bar");
		if (healthBarTransform)
		{
			Transform* backgroundTransform = TransformAPI::findChildByName(healthBarTransform, "Background");
			if (backgroundTransform)
			{
				if (!m_healthBarSlider)
				{
					Transform* slider1Transform = TransformAPI::findChildByName(backgroundTransform, "Slider 1");
					if (slider1Transform)
					{
						GameObject* slider1Object = ComponentAPI::getOwner(slider1Transform);
						m_healthBarSlider = static_cast<UISlider*>(GameObjectAPI::getComponent(slider1Object, ComponentType::UISLIDER));
					}
				}

				if (!m_healthBar2Slider)
				{
					Transform* slider2Transform = TransformAPI::findChildByName(backgroundTransform, "Slider 2");
					if (slider2Transform)
					{
						GameObject* slider2Object = ComponentAPI::getOwner(slider2Transform);
						m_healthBar2Slider = static_cast<UISlider*>(GameObjectAPI::getComponent(slider2Object, ComponentType::UISLIDER));
					}
				}
			}
		}
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

void EnemyDamageable::takeDamage(const HitContext& ctx)
{
	const EnemyHitContext& enemyCtx = static_cast<const EnemyHitContext&>(ctx);

	if (enemyCtx.attacker)
	{
		m_damageSource = enemyCtx.attacker;
	}

	Damageable::takeDamage(enemyCtx.damage);

	m_damageSource = nullptr;
}

IMPLEMENT_SCRIPT(EnemyDamageable)
