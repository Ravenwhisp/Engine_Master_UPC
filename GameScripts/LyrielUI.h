#pragma once

#include "ScriptAPI.h"
#include "CharacterUI.h"
#include "Transform2D.h"

class LyrielUI : public CharacterUI
{
	DECLARE_SCRIPT(LyrielUI)

public:
	explicit LyrielUI(GameObject* owner);

	void Start() override;

	ScriptFieldList getExposedFields() const override;

public:
	// Charged Attack
	void showChargedAttackUI();
	void updateChargedAttackUI(const Vector3& origin, const Vector3& aimDirection, float range);
	void hideChargedAttackUI();

	// Arrow Volley
	void showArrowVolleyUI();
	void updateArrowVolleyUI(const Vector3& origin, const Vector3& aimDirection);
	void hideArrowVolleyUI();

	// Dash Charges
	void setupDashCharges(int maxCharges);
	void updateDashChargesUI(int currentCharges, int maxCharges, float dt);

private:
	float moveTowards(float current, float target, float maxDelta);
	void updateChargeVisual(Transform2D* transform, float& currentScale, float targetScale, float dt);

public:
	float m_chargedScale = 1.0f;
	float m_emptyScale = 0.5f;
	float m_uiScaleSpeed = 3.0f;

private:
	// Charged Attack
	ScriptComponentRef<Transform> m_chargedAttackUI;
	Transform* m_chargedAttackUITransform = nullptr;

	// Arrow Volley
	ScriptComponentRef<Transform> m_arrowVolleyUI;
	Transform* m_arrowVolleyUITransform = nullptr;

	// Dash Charges
	ScriptComponentRef<Transform2D> m_charge1UI;
	ScriptComponentRef<Transform2D> m_charge2UI;
	ScriptComponentRef<Transform2D> m_charge3UI;

	Transform2D* m_charge1Transform2D = nullptr;
	Transform2D* m_charge2Transform2D = nullptr;
	Transform2D* m_charge3Transform2D = nullptr;

	float m_charge1Scale = 1.0f;
	float m_charge2Scale = 1.0f;
	float m_charge3Scale = 1.0f;
};