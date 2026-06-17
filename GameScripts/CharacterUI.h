#pragma once

#include "ScriptAPI.h"
#include "Transform.h"
#include "UISlider.h"

enum class AbilityUISlot
{
	BasicAttack = 0,
	ChargedAttack,
	Ability,
	Dash,
	Count
};

class CharacterUI : public Script
{
	DECLARE_SCRIPT(CharacterUI)

public:
	explicit CharacterUI(GameObject* owner);
	virtual ~CharacterUI() = default;

	void Start() override;

	ScriptFieldList getExposedFields() const override;

public:
	void showAbilityCooldown(AbilityUISlot slot);
	void hideAbilityCooldown(AbilityUISlot slot);
	void updateAbilityCooldown(AbilityUISlot slot, float ratio);

private:
	int getSlotIndex(AbilityUISlot slot) const;

private:
	ScriptComponentRef<Transform> m_basicAttackCooldownUI;
	ScriptComponentRef<UISlider> m_basicAttackCooldownSlider;

	ScriptComponentRef<Transform> m_chargedAttackCooldownUI;
	ScriptComponentRef<UISlider> m_chargedAttackCooldownSlider;

	ScriptComponentRef<Transform> m_abilityCooldownUI;
	ScriptComponentRef<UISlider> m_abilityCooldownSlider;

	ScriptComponentRef<Transform> m_dashCooldownUI;
	ScriptComponentRef<UISlider> m_dashCooldownSlider;

	GameObject* m_cooldownObjects[static_cast<int>(AbilityUISlot::Count)] = {};
	UISlider* m_cooldownSliders[static_cast<int>(AbilityUISlot::Count)] = {};
};