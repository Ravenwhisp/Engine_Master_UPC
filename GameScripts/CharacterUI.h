#pragma once

#include "ScriptAPI.h"
#include "Transform.h"
#include "Transform2D.h"
#include "UISlider.h"

enum class AbilityUISlot
{
	BasicAttack = 0,
	ChargedAttack,
	Ability,
	Dash,
	Count
};

struct CooldownData
{
    Transform2D* container = nullptr;
    Transform2D* background = nullptr;
    Transform2D* frame = nullptr;
	GameObject* disabled = nullptr;
    UISlider* slider = nullptr;
    Transform2D* glow = nullptr;

	float remainingTime = 0.0f;
};

class CharacterUI : public Script
{
    DECLARE_SCRIPT(CharacterUI)

public:
    explicit CharacterUI(GameObject* owner);
    virtual ~CharacterUI() = default;

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

public:
    void showAbilityCooldown(AbilityUISlot slot);
    void hideAbilityCooldown(AbilityUISlot slot);
    void updateAbilityCooldown(AbilityUISlot slot, float ratio);

private:
    void setReferences(AbilityUISlot slot, Transform* transform);
    int getSlotIndex(AbilityUISlot slot) const;

private:
    ScriptComponentRef<Transform> m_basicAttackCooldownUI;
    ScriptComponentRef<Transform> m_chargedAttackCooldownUI;
    ScriptComponentRef<Transform> m_abilityCooldownUI;
    ScriptComponentRef<Transform> m_dashCooldownUI;

    float m_cooldownEndTime = 1.0f;

    CooldownData m_cooldowns[static_cast<int>(AbilityUISlot::Count)];
};