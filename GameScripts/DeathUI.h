#pragma once

#include "CharacterUI.h"
#include "Transform.h"
#include "UISlider.h"

class DeathUI : public CharacterUI
{
	DECLARE_SCRIPT(DeathUI)

public:
	explicit DeathUI(GameObject* owner);

	void Start() override;

	ScriptFieldList getExposedFields() const override;

public:
	// Taunt
	void showTauntUI();
	void updateTauntUI(const Vector3& origin, const Vector3& aimDirection);
	void hideTauntUI();

	// Charged Attack
	void showChargedAttackUI();
	void updateChargedAttackUI(const Vector3& origin, const Vector3& aimDirection);
	void hideChargedAttackUI();

	// Slash Combo
	void setupSlashUI();
	void updateBasicSlashUI(float attackStateTimer, float attackLockDuration);
	void updateChargedSlashUI(float attackStateTimer, float attackLockDuration);
	void hideSlashUI();

private:
	void updateSlashUI(Transform* slashTransform, UISlider* slashSlider, float attackStateTimer, float attackLockDuration);

private:
	// Taunt
	ScriptComponentRef<Transform> m_tauntUI;
	Transform* m_tauntUITransform = nullptr;

	// Charged Attack
	ScriptComponentRef<Transform> m_chargedAttackUI;
	Transform* m_chargedAttackUITransform = nullptr;

	// Slash Combo
	ScriptComponentRef<Transform> m_basicSlashUI;
	ScriptComponentRef<UISlider> m_basicSlashSlider;

	ScriptComponentRef<Transform> m_chargedSlashUI;
	ScriptComponentRef<UISlider> m_chargedSlashSlider;

	Transform* m_basicSlashUITransform = nullptr;
	UISlider* m_basicSlashUISlider = nullptr;

	Transform* m_chargedSlashUITransform = nullptr;
	UISlider* m_chargedSlashUISlider = nullptr;
};