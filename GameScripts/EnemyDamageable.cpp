#include "pch.h"
#include "EnemyDamageable.h"

#include "EnemyDetectionAggro.h"
#include "EnemySound.h"
#include "Transform2D.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(EnemyDamageable, Damageable,
	SERIALIZED_COMPONENT_REF(m_healthBarContainer, "Health Bar Container", ComponentType::TRANSFORM2D),
	SERIALIZED_FLOAT(m_healthBarFadeTime, "Health Bar Fade Time", 0.0f, 5.0f, 0.05f)
)

EnemyDamageable::EnemyDamageable(GameObject* owner)
	: Damageable(owner)
{
}

void EnemyDamageable::Start()
{
	resolveHealthBarReferences();

	Damageable::Start();

	m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(m_owner);

	if (!m_enemyDetectionAggro)
	{
		Debug::warn("EnemyDetectionAggro Script is missing from %s", GameObjectAPI::getName(m_owner));
	}

	m_enemySound = GameObjectAPI::findScript<EnemySound>(m_owner);

	if (!m_healthBarContainerTransform)
	{
		Debug::warn("Health Bar Container Transform2D is missing from %s", GameObjectAPI::getName(m_owner));
		return;
	}

	setHealthBarAlpha(0.0f);
}

void EnemyDamageable::Update()
{
	Damageable::Update();
	updateHealthBarFade();
}

void EnemyDamageable::onDamaged(float amount)
{
	Damageable::onDamaged(amount);

	if (!m_healthBarFadeActive && m_healthBarFadeTimer < m_healthBarFadeTime)
	{
		m_healthBarFadeActive = true;
	}

	if (m_enemySound)
	{
		m_enemySound->playHurt();
	}

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

void EnemyDamageable::resolveHealthBarReferences() 
{
	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

	Transform* healthBarTransform = TransformAPI::findChildByName(ownerTransform, "Health Bar");
	if (healthBarTransform)
	{
		Transform* backgroundTransform = TransformAPI::findChildByName(healthBarTransform, "Background");
		if (backgroundTransform)
		{
			GameObject* backgroundObject = ComponentAPI::getOwner(backgroundTransform);

			if (!m_healthBarContainerTransform)
			{
				m_healthBarContainerTransform = static_cast<Transform2D*>(GameObjectAPI::getComponent(backgroundObject, ComponentType::TRANSFORM2D));
			}

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

	if (!m_healthBarContainerTransform)
	{
		m_healthBarContainerTransform = m_healthBarContainer.getReferencedComponent();
	}
}

void EnemyDamageable::updateHealthBarFade()
{
	if (!m_healthBarFadeActive)
	{
		return;
	}

	if (!m_healthBarContainerTransform)
	{
		m_healthBarFadeActive = false;
		return;
	}

	if (m_healthBarFadeTime <= 0.0f)
	{
		setHealthBarAlpha(1.0f);
		m_healthBarFadeActive = false;
		return;
	}

	m_healthBarFadeTimer += Time::getDeltaTime();

	float t = m_healthBarFadeTimer / m_healthBarFadeTime;
	t = std::clamp(t, 0.0f, 1.0f);

	float alpha = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, t);

	setHealthBarAlpha(alpha);

	if (t >= 1.0f)
	{
		setHealthBarAlpha(1.0f);
		m_healthBarFadeActive = false;
	}
}

void EnemyDamageable::setHealthBarAlpha(float alpha)
{
	if (!m_healthBarContainerTransform)
	{
		return;
	}

	alpha = std::clamp(alpha, 0.0f, 1.0f);

	Transform2DAPI::setAlpha(m_healthBarContainerTransform, alpha);
}

IMPLEMENT_SCRIPT(EnemyDamageable)
