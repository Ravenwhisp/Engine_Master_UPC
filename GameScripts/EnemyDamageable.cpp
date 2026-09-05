#include "pch.h"
#include "EnemyDamageable.h"

#include "EnemyDetectionAggro.h"
#include "EnemySound.h"
#include "EnemyBaseController.h"
#include "EnemyBaseDataConfig.h"
#include "EnemyShadowMark.h"
#include "BarrierComponent.h"
#include "Transform2D.h"
#include "ReaperGauge.h"
#include "ShadowExecution.h"
#include "PersistingCheckpointState.h"

#include <algorithm>

IMPLEMENT_SCRIPT_FIELDS_INHERITED(EnemyDamageable, Damageable,
	FIELD_GROUP_LABEL("Health Bar"),
	SERIALIZED_COMPONENT_REF(m_healthBarContainer, "Health Bar Container", ComponentType::TRANSFORM2D),
	SERIALIZED_FLOAT(m_healthBarFadeTime, "Health Bar Fade Time", 0.0f, 5.0f, 0.05f),
	FIELD_GROUP_LABEL("Shadow Execution Preview"),
	SERIALIZED_COMPONENT_REF(m_shadowExecutionPreview, "Shadow Execution Preview", ComponentType::UISLIDER),
	SERIALIZED_COMPONENT_REF(m_shadowExecutionThresholdMarker, "Shadow Execution Threshold Marker", ComponentType::TRANSFORM2D),
	SERIALIZED_FLOAT(m_shadowExecutionPreviewFadeTime, "Shadow Preview Fade Time", 0.0f, 2.0f, 0.05f),
	SERIALIZED_FLOAT(m_shadowExecutionPreviewHitTime, "Shadow Preview Hit Time", 0.05f, 1.0f, 0.05f),
	SERIALIZED_FLOAT(m_shadowExecutionPreviewNonLethalAlpha, "Shadow Preview Non-Lethal Alpha", 0.0f, 1.0f, 0.05f),
	SERIALIZED_FLOAT(m_shadowExecutionPreviewLethalAlpha, "Shadow Preview Lethal Alpha", 0.0f, 1.0f, 0.05f),
	FIELD_GROUP_LABEL("DissolveEffect"),
	SERIALIZED_FLOAT(m_dissolveDuration, "Dissolve Duration", 0.1f, 5.0f, 0.0f)
)

EnemyDamageable::EnemyDamageable(GameObject* owner)
	: Damageable(owner)
{
}

void EnemyDamageable::Start()
{
	if (!PersistingCheckpointState::Get().IsStartOfLevel())
	{
		std::vector<UID>* deadEnemies = &PersistingCheckpointState::Get().m_deadEnemiesPersistent;

		if (std::find(deadEnemies->begin(), deadEnemies->end(), m_owner->GetID()) != deadEnemies->end())
		{
			GameObjectAPI::removeGameObject(m_owner);
			return;
		}
	}

	resolveHealthBarReferences();
	resolveReaperGauge();
	resolveShadowExecution();

	if (m_shadowExecutionPreviewSlider)
	{
		SliderAPI::setFillMethod(m_shadowExecutionPreviewSlider, FillMethod::Horizontal);
		SliderAPI::setFillOrigin(m_shadowExecutionPreviewSlider, FillOrigin::HorizontalLeft);
		SliderAPI::setFillAmountVec(m_shadowExecutionPreviewSlider, Vector2(0.0f, 0.0f));
	}

	if (m_shadowExecutionPreviewTransform)
	{
		m_shadowExecutionPreviewBaseScale = Transform2DAPI::getScale(m_shadowExecutionPreviewTransform);
		Transform2DAPI::setAlpha(m_shadowExecutionPreviewTransform, 0.0f);
	}

	if (m_shadowExecutionThresholdMarkerTransform)
	{
		Transform2DAPI::setAlpha(m_shadowExecutionThresholdMarkerTransform, 0.0f);
	}

	// Override HP from controller's attack config (inherits from EnemyBaseDataConfig)
	EnemyBaseController* controller = GameObjectAPI::findScript<EnemyBaseController>(m_owner);
	if (controller)
	{
		m_baseDataConfig = controller->getBaseDataConfig();

		if (m_baseDataConfig && m_baseDataConfig->m_maxHp > 0.0f)
		{
			m_maxHp = m_baseDataConfig->m_maxHp;
		}
	}

	Damageable::Start();

	m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(m_owner);

	if (!m_enemyDetectionAggro)
	{
		Debug::warn("EnemyDetectionAggro Script is missing from %s", GameObjectAPI::getName(m_owner));
	}

	m_shadowMark = GameObjectAPI::findScript<EnemyShadowMark>(m_owner);

	m_enemySound = GameObjectAPI::findScript<EnemySound>(m_owner);

	if (!m_healthBarContainerTransform)
	{
		Debug::warn("Health Bar Container Transform2D is missing from %s", GameObjectAPI::getName(m_owner));
		return;
	}

	setHealthBarAlpha(0.0f);

	loadDissolveComponent();
}

void EnemyDamageable::Update()
{
	Damageable::Update();
	updateHealthBarFade();
	updateShadowExecutionPreviewAvailability();
	updateShadowExecutionPreviewAnimation(Time::getDeltaTime());

	if (m_dissolve != nullptr && m_dissolveActive)
	{
		updateDissolveEffect();
	}
}

void EnemyDamageable::takeDamage(const HitContext& ctx)
{
	const EnemyHitContext& enemyCtx = static_cast<const EnemyHitContext&>(ctx);

	resetLastShadowMarkResult();

	if (m_isDead || m_invulnerable)
	{
		return;
	}

	processShadowMarkHit(enemyCtx.attackType);

	auto* barrier = GameObjectAPI::findScript<BarrierComponent>(m_owner);
	if (barrier && barrier->hasActiveBarriers())
	{
		BarrierResult result = barrier->processBarrierDamage(enemyCtx.damage, m_currentHp, m_maxHp, m_lastHitExploitedShadowMark);

		if (result.blocked)
		{
			return;
		}

		if (result.damageToApply > 0.0f)
		{
			EnemyHitContext cappedHit = enemyCtx;
			cappedHit.damage = result.damageToApply;
			applyDamageWithoutShadowMark(cappedHit);
		}

		return;
	}

	applyDamageWithoutShadowMark(enemyCtx);
}

float EnemyDamageable::getShadowExecutionThresholdMultiplier() const
{
	return m_baseDataConfig ? m_baseDataConfig->m_shadowExecutionThresholdMultiplier : 1.0f;
}

void EnemyDamageable::playShadowExecutionHitPreview()
{
	if (!m_shadowExecutionPreviewActive || !m_shadowExecutionPreviewSlider || !m_shadowExecutionPreviewTransform)
	{
		return;
	}

	const Vector2 previewRange = SliderAPI::getFillAmountVec(m_shadowExecutionPreviewSlider);

	m_shadowExecutionPreviewHitStart = previewRange.y;
	m_shadowExecutionPreviewHitEnd = previewRange.x;
	m_shadowExecutionPreviewHitTimer = 0.0f;
	m_shadowExecutionPreviewHitAnimating = true;

	Transform2DAPI::setAlpha(m_shadowExecutionPreviewTransform, m_shadowExecutionPreviewLethal ? m_shadowExecutionPreviewLethalAlpha : m_shadowExecutionPreviewNonLethalAlpha);
}

void EnemyDamageable::startDissolve()
{
	if (!m_dissolve)
	{
		return;
	}

	m_dissolveTimer = 0.0f;
	m_dissolveActive = true;
}

bool EnemyDamageable::isDissolveFinished() const
{
	return !m_dissolve || m_dissolveTimer >= m_dissolveDuration;
}

void EnemyDamageable::kill()
{
	auto* barrier = GameObjectAPI::findScript<BarrierComponent>(m_owner);
	if (barrier && barrier->hasActiveBarriers())
	{
		Debug::log("[Barrier] %s kill prevented by active barrier.", GameObjectAPI::getName(m_owner));
		return;
	}

	Damageable::kill();
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

	if (m_shadowExecutionPreviewActive && !m_shadowExecutionPreviewHitAnimating)
	{
		updateShadowExecutionPreview();
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

void EnemyDamageable::onDeath()
{
	Damageable::onDeath();

	setShadowExecutionThresholdMarkerVisible(false);

	if (m_shadowMark)
	{
		m_shadowMark->clearMark();
	}
	
	PersistingCheckpointState::Get().m_deadEnemies.push_back(m_owner->GetID());
}

bool EnemyDamageable::processShadowMarkHit(PlayerAttackType attackType)
{
	if (!m_shadowMark)
	{
		return false;
	}

	m_lastHitExploitedShadowMark = m_shadowMark->processAttack(attackType);
	return m_lastHitExploitedShadowMark;
}

void EnemyDamageable::applyDamageWithoutShadowMark(const EnemyHitContext& hit)
{
	if (hit.attacker)
	{
		m_damageSource = hit.attacker;
	}

	Damageable::takeDamage(hit);

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

			if (!m_shadowExecutionPreviewSlider || !m_shadowExecutionPreviewTransform)
			{
				Transform* previewTransform = TransformAPI::findChildByName(backgroundTransform, "Shadow Execution Preview");
				if (previewTransform)
				{
					GameObject* previewObject = ComponentAPI::getOwner(previewTransform);

					if (!m_shadowExecutionPreviewSlider)
					{
						m_shadowExecutionPreviewSlider = static_cast<UISlider*>(GameObjectAPI::getComponent(previewObject, ComponentType::UISLIDER));
					}

					if (!m_shadowExecutionPreviewTransform)
					{
						m_shadowExecutionPreviewTransform = static_cast<Transform2D*>(GameObjectAPI::getComponent(previewObject, ComponentType::TRANSFORM2D));
					}
				}
			}

			if (!m_shadowExecutionThresholdMarkerTransform)
			{
				Transform* markerTransform = TransformAPI::findChildByName(backgroundTransform, "Shadow Execution Threshold Marker");
				if (markerTransform)
				{
					GameObject* markerObject = ComponentAPI::getOwner(markerTransform);
					m_shadowExecutionThresholdMarkerTransform = static_cast<Transform2D*>(GameObjectAPI::getComponent(markerObject, ComponentType::TRANSFORM2D));
				}
			}
		}
	}

	if (!m_healthBarContainerTransform)
	{
		m_healthBarContainerTransform = m_healthBarContainer.getReferencedComponent();
	}

	if (!m_shadowExecutionPreviewSlider)
	{
		m_shadowExecutionPreviewSlider = m_shadowExecutionPreview.getReferencedComponent();
	}

	if (!m_shadowExecutionPreviewTransform && m_shadowExecutionPreviewSlider)
	{
		GameObject* previewObject = ComponentAPI::getOwner(m_shadowExecutionPreviewSlider);
		m_shadowExecutionPreviewTransform = static_cast<Transform2D*>(GameObjectAPI::getComponent(previewObject, ComponentType::TRANSFORM2D));
	}

	if (!m_shadowExecutionThresholdMarkerTransform)
	{
		m_shadowExecutionThresholdMarkerTransform = m_shadowExecutionThresholdMarker.getReferencedComponent();
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

void EnemyDamageable::updateDissolveEffect()
{
	m_dissolveTimer += (Time::getDeltaTime());
	if (m_dissolveTimer >= m_dissolveDuration)
	{
		m_dissolveTimer = m_dissolveDuration;
		m_dissolveActive = false;
	}

	float amount = m_dissolveTimer / m_dissolveDuration;

	ShadersAPI::setDissolveAmount(m_dissolve, amount);
}

void EnemyDamageable::setHealthBarAlpha(float alpha)
{
	if (!m_healthBarContainerTransform)
	{
		return;
	}

	alpha = std::clamp(alpha, 0.0f, 1.0f);

	Transform2DAPI::setAlpha(m_healthBarContainerTransform, alpha);

	auto* barrier = GameObjectAPI::findScript<BarrierComponent>(m_owner);
	if (barrier)
	{
		barrier->setBarrierUIAlpha(alpha);
	}
}

void EnemyDamageable::resolveReaperGauge()
{
	const std::vector<GameObject*> holders = SceneAPI::findAllGameObjectsWithScript<ReaperGauge>();

	if (!holders.empty())
	{
		m_reaperGauge = GameObjectAPI::findScript<ReaperGauge>(holders[0]);
	}

	if (!m_reaperGauge)
	{
		Debug::warn("[EnemyDamageable] ReaperGauge not found for '%s'.", GameObjectAPI::getName(m_owner));
	}
}

void EnemyDamageable::updateShadowExecutionPreviewAvailability()
{
	if (!m_reaperGauge || !m_shadowExecution)
	{
		return;
	}

	const bool shadowExecutionAvailable = m_reaperGauge->isFull();
	const bool shadowExecutionPlaying = m_shadowExecution->isActive();
	const bool shouldBeActive = (shadowExecutionAvailable || shadowExecutionPlaying) && !m_isDead;

	if (shouldBeActive != m_shadowExecutionPreviewActive)
	{
		setShadowExecutionPreviewActive(shouldBeActive);
	}
}

void EnemyDamageable::setShadowExecutionPreviewActive(bool active)
{
	m_shadowExecutionPreviewActive = active;

	if (active)
	{
		m_shadowExecutionPreviewFadeTimer = 0.0f;
		m_shadowExecutionPreviewHitTimer = 0.0f;
		m_shadowExecutionPreviewHitAnimating = false;

		if (m_shadowExecutionPreviewTransform)
		{
			Transform2DAPI::setAlpha(m_shadowExecutionPreviewTransform, 0.0f);
			Transform2DAPI::setScale(m_shadowExecutionPreviewTransform, m_shadowExecutionPreviewBaseScale);
		}

		updateShadowExecutionPreview();
		updateShadowExecutionThresholdMarker();
		setShadowExecutionThresholdMarkerVisible(true);
	}
	else
	{
		resetShadowExecutionPreviewVisual();
		setShadowExecutionThresholdMarkerVisible(false);
	}
}

void EnemyDamageable::resolveShadowExecution()
{
	const std::vector<GameObject*> holders = SceneAPI::findAllGameObjectsWithScript<ShadowExecution>();

	if (!holders.empty())
	{
		m_shadowExecution = GameObjectAPI::findScript<ShadowExecution>(holders[0]);
	}

	if (!m_shadowExecution)
	{
		Debug::warn("[EnemyDamageable] ShadowExecution not found for '%s'.", GameObjectAPI::getName(m_owner));
	}
}

void EnemyDamageable::updateShadowExecutionPreview()
{
	if (!m_shadowExecutionPreviewActive || !m_shadowExecution || !m_shadowExecutionPreviewSlider || m_isDead || m_shadowExecutionPreviewHitAnimating)
	{
		return;
	}

	const float maxHp = getMaxHp();
	const float currentHp = getCurrentHp();

	if (maxHp <= 0.0f || currentHp <= 0.0f)
	{
		resetShadowExecutionPreviewVisual();
		return;
	}

	const ShadowExecutionPreview preview = m_shadowExecution->calculatePreview(this);

	float currentHpPercent = currentHp / maxHp;
	float resultingHpPercent = preview.resultingHpPercent;

	currentHpPercent = std::clamp(currentHpPercent, 0.0f, 1.0f);
	resultingHpPercent = std::clamp(resultingHpPercent, 0.0f, currentHpPercent);

	if (currentHpPercent <= resultingHpPercent)
	{
		resetShadowExecutionPreviewVisual();
		return;
	}

	m_shadowExecutionPreviewLethal = preview.willDie;

	SliderAPI::setFillAmountVec(m_shadowExecutionPreviewSlider, Vector2(resultingHpPercent, currentHpPercent));
}

void EnemyDamageable::updateShadowExecutionPreviewAnimation(float dt)
{
	if (!m_shadowExecutionPreviewActive || !m_shadowExecutionPreviewSlider || !m_shadowExecutionPreviewTransform)
	{
		return;
	}

	if (m_shadowExecutionPreviewHitAnimating)
	{
		m_shadowExecutionPreviewHitTimer += dt;

		float t = m_shadowExecutionPreviewHitTime > 0.0f ? m_shadowExecutionPreviewHitTimer / m_shadowExecutionPreviewHitTime : 1.0f;
		t = std::clamp(t, 0.0f, 1.0f);

		const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, t);
		const float currentEnd = m_shadowExecutionPreviewHitStart + (m_shadowExecutionPreviewHitEnd - m_shadowExecutionPreviewHitStart) * easedT;

		SliderAPI::setFillAmountVec(m_shadowExecutionPreviewSlider, Vector2(m_shadowExecutionPreviewHitEnd, currentEnd));

		const float popStrength = m_shadowExecutionPreviewLethal ? 0.15f : 0.08f;
		const float pop = 1.0f + sinf(t * 3.14159265f) * popStrength;

		Transform2DAPI::setScale(m_shadowExecutionPreviewTransform, Vector2(m_shadowExecutionPreviewBaseScale.x * pop, m_shadowExecutionPreviewBaseScale.y * pop));
		Transform2DAPI::setAlpha(m_shadowExecutionPreviewTransform, m_shadowExecutionPreviewLethal ? m_shadowExecutionPreviewLethalAlpha : m_shadowExecutionPreviewNonLethalAlpha);

		if (t >= 1.0f)
		{
			m_shadowExecutionPreviewHitAnimating = false;
			SliderAPI::setFillAmountVec(m_shadowExecutionPreviewSlider, Vector2(0.0f, 0.0f));
			Transform2DAPI::setScale(m_shadowExecutionPreviewTransform, m_shadowExecutionPreviewBaseScale);
			Transform2DAPI::setAlpha(m_shadowExecutionPreviewTransform, 0.0f);
		}

		return;
	}

	m_shadowExecutionPreviewFadeTimer += dt;

	float fadeT = m_shadowExecutionPreviewFadeTime > 0.0f ? m_shadowExecutionPreviewFadeTimer / m_shadowExecutionPreviewFadeTime : 1.0f;
	fadeT = std::clamp(fadeT, 0.0f, 1.0f);

	const float easedFade = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, fadeT);
	const float targetAlpha = m_shadowExecutionPreviewLethal ? m_shadowExecutionPreviewLethalAlpha : m_shadowExecutionPreviewNonLethalAlpha;

	Transform2DAPI::setAlpha(m_shadowExecutionPreviewTransform, targetAlpha * easedFade);
}

void EnemyDamageable::resetShadowExecutionPreviewVisual()
{
	m_shadowExecutionPreviewFadeTimer = 0.0f;
	m_shadowExecutionPreviewHitTimer = 0.0f;
	m_shadowExecutionPreviewHitAnimating = false;

	if (m_shadowExecutionPreviewSlider)
	{
		SliderAPI::setFillAmountVec(m_shadowExecutionPreviewSlider, Vector2(0.0f, 0.0f));
	}

	if (m_shadowExecutionPreviewTransform)
	{
		Transform2DAPI::setAlpha(m_shadowExecutionPreviewTransform, 0.0f);
		Transform2DAPI::setScale(m_shadowExecutionPreviewTransform, m_shadowExecutionPreviewBaseScale);
	}
}

void EnemyDamageable::setShadowExecutionThresholdMarkerVisible(bool visible)
{
	if (!m_shadowExecutionThresholdMarkerTransform)
	{
		return;
	}

	Transform2DAPI::setAlpha(m_shadowExecutionThresholdMarkerTransform, visible ? 1.0f : 0.0f);
}

void EnemyDamageable::updateShadowExecutionThresholdMarker()
{
	if (!m_shadowExecutionThresholdMarkerTransform || !m_healthBarContainerTransform || !m_shadowExecution)
	{
		return;
	}

	const float thresholdPercent = m_shadowExecution->getExecutionThresholdPercent(this);

	const Vector2 containerSize = Transform2DAPI::getBaseSize(m_healthBarContainerTransform);
	const Vector2 containerPivot = Transform2DAPI::getPivot(m_healthBarContainerTransform);
	const Vector2 markerPosition = Transform2DAPI::getPosition(m_shadowExecutionThresholdMarkerTransform);

	const float leftEdge = -containerSize.x * containerPivot.x;
	const float markerX = leftEdge + containerSize.x * thresholdPercent;

	Transform2DAPI::setPosition(m_shadowExecutionThresholdMarkerTransform, Vector2(markerX, markerPosition.y));
}

void EnemyDamageable::loadDissolveComponent()
{
	m_dissolve = findDissolveInHierarchy(GameObjectAPI::getTransform(m_owner));

	if (!m_dissolve)
	{
		Debug::warn("[EnemyDamageable] Dissolve component not found in hierarchy of '%s'.", GameObjectAPI::getName(m_owner));
	}
}

DissolveComponent* EnemyDamageable::findDissolveInHierarchy(Transform* transform)
{
	if (!transform)
	{
		return nullptr;
	}

	GameObject* object = ComponentAPI::getOwner(transform);

	if (DissolveComponent* dissolve = ShadersAPI::getDissolveComponent(object))
	{
		return dissolve;
	}

	const int childCount = TransformAPI::getChildCount(transform);

	for (int i = 0; i < childCount; ++i)
	{
		Transform* child = TransformAPI::getChild(transform, i);

		if (DissolveComponent* dissolve = findDissolveInHierarchy(child))
		{
			return dissolve;
		}
	}

	return nullptr;
}

IMPLEMENT_SCRIPT(EnemyDamageable)
