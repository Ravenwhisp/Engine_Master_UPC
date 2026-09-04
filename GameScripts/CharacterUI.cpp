#include "pch.h"
#include "CharacterUI.h"

IMPLEMENT_SCRIPT_FIELDS(CharacterUI,
	FIELD_GROUP_COLLAPSE("Ability Cooldowns",
		SERIALIZED_COMPONENT_REF(m_basicAttackCooldownUI, "Basic Attack Cooldown UI", ComponentType::TRANSFORM),

		SERIALIZED_COMPONENT_REF(m_chargedAttackCooldownUI, "Charged Attack Cooldown UI", ComponentType::TRANSFORM),

		SERIALIZED_COMPONENT_REF(m_abilityCooldownUI, "Ability Cooldown UI", ComponentType::TRANSFORM),

		SERIALIZED_COMPONENT_REF(m_dashCooldownUI, "Dash Cooldown UI", ComponentType::TRANSFORM),

		SERIALIZED_FLOAT(m_cooldownEndTime, "Cooldown End Time", 0.f, 10.f, 1.f)
	)
)

CharacterUI::CharacterUI(GameObject* owner)
	: Script(owner)
{
}

void CharacterUI::Start()
{
	Transform* basicAttackUI = m_basicAttackCooldownUI.getReferencedComponent();
	Transform* chargedAttackUI = m_chargedAttackCooldownUI.getReferencedComponent();
	Transform* abilityUI = m_abilityCooldownUI.getReferencedComponent();
	Transform* dashUI = m_dashCooldownUI.getReferencedComponent();

	setReferences(AbilityUISlot::BasicAttack, basicAttackUI);
	setReferences(AbilityUISlot::ChargedAttack, chargedAttackUI);
	setReferences(AbilityUISlot::Ability, abilityUI);
	setReferences(AbilityUISlot::Dash, dashUI);

	for (auto& cooldown : m_cooldowns)
	{
		if (cooldown.disabled)
		{
			GameObjectAPI::setActive(cooldown.disabled, false);
			Transform2DAPI::setAlpha(cooldown.background, 1.f);
			Transform2DAPI::setAlpha(cooldown.glow, 0.f);
		}
	}
}

void CharacterUI::setReferences(AbilityUISlot slot, Transform* transform) 
{
	if (!transform)	
	{
		return;
	}

	Transform* containerTransform = TransformAPI::findChildByName(transform, "Container");
	if (containerTransform)
	{
		m_cooldowns[static_cast<int>(slot)].container = static_cast<Transform2D*>(GameObjectAPI::getComponent(ComponentAPI::getOwner(containerTransform), ComponentType::TRANSFORM2D));
		Transform* backgroundTransform = TransformAPI::findChildByName(containerTransform, "Background");
		if (backgroundTransform)
		{
			m_cooldowns[static_cast<int>(slot)].background = static_cast<Transform2D*>(GameObjectAPI::getComponent(ComponentAPI::getOwner(backgroundTransform), ComponentType::TRANSFORM2D));
			Transform* frameTransform = TransformAPI::findChildByName(backgroundTransform, "Frame");
			if (frameTransform)
			{
				m_cooldowns[static_cast<int>(slot)].frame = static_cast<Transform2D*>(GameObjectAPI::getComponent(ComponentAPI::getOwner(frameTransform), ComponentType::TRANSFORM2D));
			}
		}
		Transform* disabledTransform = TransformAPI::findChildByName(containerTransform, "Disabled");
		if (disabledTransform)
		{
			m_cooldowns[static_cast<int>(slot)].disabled = disabledTransform->getOwner();
			Transform* sliderTransform = TransformAPI::findChildByName(disabledTransform, "Slider");
			if (sliderTransform)
			{
				m_cooldowns[static_cast<int>(slot)].slider = static_cast<UISlider*>(GameObjectAPI::getComponent(ComponentAPI::getOwner(sliderTransform), ComponentType::UISLIDER));
			}
		}
		Transform* glowTransform = TransformAPI::findChildByName(containerTransform, "Glow");
		if (glowTransform)
		{
			m_cooldowns[static_cast<int>(slot)].glow = static_cast<Transform2D*>(GameObjectAPI::getComponent(ComponentAPI::getOwner(glowTransform), ComponentType::TRANSFORM2D));
		}
	}
}

void CharacterUI::Update()
{
	for (auto& cooldown : m_cooldowns)
	{
		if (cooldown.remainingTime > 0.0f)
		{
			cooldown.remainingTime -= Time::getDeltaTime();
			const float invt = cooldown.remainingTime / m_cooldownEndTime;
			const float t = 1 - invt;

			const float easedt = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, t);
			if (cooldown.container)
			{
				const float containerScale = MathAPI::lerp(1.05f, 1, easedt);
				Transform2DAPI::setScale(cooldown.container, Vector2(containerScale, containerScale));
			}
			if (cooldown.frame)
			{
				Transform2DAPI::setAlpha(cooldown.frame, invt);
				const float frameScale = MathAPI::lerp(0.5f, 1.15, easedt);
				Transform2DAPI::setScale(cooldown.frame, Vector2(frameScale, frameScale));
			}

			if (cooldown.glow)
			{
				const float glowEasedAlpha = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInCubic, invt);
				Transform2DAPI::setAlpha(cooldown.glow, glowEasedAlpha);
			}
		}
	}
}

int CharacterUI::getSlotIndex(AbilityUISlot slot) const
{
	return static_cast<int>(slot);
}

void CharacterUI::showAbilityCooldown(AbilityUISlot slot)
{
	const int index = getSlotIndex(slot);

	if (index < 0 || index >= static_cast<int>(AbilityUISlot::Count))
	{
		return;
	}

	if (m_cooldowns[index].disabled)
	{
		GameObjectAPI::setActive(m_cooldowns[index].disabled, true);
		Transform2DAPI::setAlpha(m_cooldowns[index].background, 0.3f);
	}
}

void CharacterUI::hideAbilityCooldown(AbilityUISlot slot)
{
	const int index = getSlotIndex(slot);

	if (index < 0 || index >= static_cast<int>(AbilityUISlot::Count))
	{
		return;
	}

	if (m_cooldowns[index].disabled)
	{
		GameObjectAPI::setActive(m_cooldowns[index].disabled, false);
		Transform2DAPI::setAlpha(m_cooldowns[index].background, 1.f);
	}

	m_cooldowns[index].remainingTime = m_cooldownEndTime;
}

void CharacterUI::updateAbilityCooldown(AbilityUISlot slot, float ratio)
{
	const int index = getSlotIndex(slot);

	if (index < 0 || index >= static_cast<int>(AbilityUISlot::Count))
	{
		return;
	}

	if (m_cooldowns[index].slider)
	{
		SliderAPI::setFillAmount(m_cooldowns[index].slider, std::clamp(ratio, 0.0f, 1.0f));
	}
}

IMPLEMENT_SCRIPT(CharacterUI)