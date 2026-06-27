#include "pch.h"
#include "CharacterUI.h"

IMPLEMENT_SCRIPT_FIELDS(CharacterUI,
	FIELD_GROUP_COLLAPSE("Ability Cooldowns",
		SERIALIZED_COMPONENT_REF(m_basicAttackCooldownUI, "Basic Attack Cooldown UI", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_basicAttackCooldownSlider, "Basic Attack Cooldown Slider", ComponentType::UISLIDER),

		SERIALIZED_COMPONENT_REF(m_chargedAttackCooldownUI, "Charged Attack Cooldown UI", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_chargedAttackCooldownSlider, "Charged Attack Cooldown Slider", ComponentType::UISLIDER),

		SERIALIZED_COMPONENT_REF(m_abilityCooldownUI, "Ability Cooldown UI", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_abilityCooldownSlider, "Ability Cooldown Slider", ComponentType::UISLIDER),

		SERIALIZED_COMPONENT_REF(m_dashCooldownUI, "Dash Cooldown UI", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_dashCooldownSlider, "Dash Cooldown Slider", ComponentType::UISLIDER)
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

	m_cooldownObjects[getSlotIndex(AbilityUISlot::BasicAttack)] = basicAttackUI ? basicAttackUI->getOwner() : nullptr;
	m_cooldownObjects[getSlotIndex(AbilityUISlot::ChargedAttack)] = chargedAttackUI ? chargedAttackUI->getOwner() : nullptr;
	m_cooldownObjects[getSlotIndex(AbilityUISlot::Ability)] = abilityUI ? abilityUI->getOwner() : nullptr;
	m_cooldownObjects[getSlotIndex(AbilityUISlot::Dash)] = dashUI ? dashUI->getOwner() : nullptr;

	m_cooldownSliders[getSlotIndex(AbilityUISlot::BasicAttack)] = m_basicAttackCooldownSlider.getReferencedComponent();
	m_cooldownSliders[getSlotIndex(AbilityUISlot::ChargedAttack)] = m_chargedAttackCooldownSlider.getReferencedComponent();
	m_cooldownSliders[getSlotIndex(AbilityUISlot::Ability)] = m_abilityCooldownSlider.getReferencedComponent();
	m_cooldownSliders[getSlotIndex(AbilityUISlot::Dash)] = m_dashCooldownSlider.getReferencedComponent();

	for (int i = 0; i < static_cast<int>(AbilityUISlot::Count); ++i)
	{
		if (m_cooldownObjects[i])
		{
			GameObjectAPI::setActive(m_cooldownObjects[i], false);
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

	if (m_cooldownObjects[index])
	{
		GameObjectAPI::setActive(m_cooldownObjects[index], true);
	}
}

void CharacterUI::hideAbilityCooldown(AbilityUISlot slot)
{
	const int index = getSlotIndex(slot);

	if (index < 0 || index >= static_cast<int>(AbilityUISlot::Count))
	{
		return;
	}

	if (m_cooldownObjects[index])
	{
		GameObjectAPI::setActive(m_cooldownObjects[index], false);
	}
}

void CharacterUI::updateAbilityCooldown(AbilityUISlot slot, float ratio)
{
	const int index = getSlotIndex(slot);

	if (index < 0 || index >= static_cast<int>(AbilityUISlot::Count))
	{
		return;
	}

	if (m_cooldownSliders[index])
	{
		SliderAPI::setFillAmount(m_cooldownSliders[index], std::clamp(ratio, 0.0f, 1.0f));
	}
}

IMPLEMENT_SCRIPT(CharacterUI)