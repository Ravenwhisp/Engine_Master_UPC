#include "pch.h"
#include "LyrielUI.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(LyrielUI, CharacterUI,
	FIELD_GROUP_LABEL("Charged Attack"),
	SERIALIZED_COMPONENT_REF(m_chargedAttackUI, "Charged Attack UI", ComponentType::TRANSFORM),

	FIELD_GROUP_LABEL("Arrow Volley"),
	SERIALIZED_COMPONENT_REF(m_arrowVolleyUI, "Arrow Volley UI", ComponentType::TRANSFORM),

	FIELD_GROUP_LABEL("Dash"),
	SERIALIZED_COMPONENT_REF(m_charge1UI, "Charge 1 UI", ComponentType::TRANSFORM2D),
	SERIALIZED_COMPONENT_REF(m_charge2UI, "Charge 2 UI", ComponentType::TRANSFORM2D),
	SERIALIZED_COMPONENT_REF(m_charge3UI, "Charge 3 UI", ComponentType::TRANSFORM2D),
	SERIALIZED_FLOAT(m_chargedScale, "Charged Scale", 0.1f, 5.0f, 0.1f),
	SERIALIZED_FLOAT(m_emptyScale, "Empty Scale", 0.1f, 5.0f, 0.1f),
	SERIALIZED_FLOAT(m_uiScaleSpeed, "UI Scale Speed", 0.1f, 20.0f, 0.1f)
)

LyrielUI::LyrielUI(GameObject* owner)
	: CharacterUI(owner)
{
}

void LyrielUI::Start()
{
	CharacterUI::Start();

	m_chargedAttackUITransform = m_chargedAttackUI.getReferencedComponent();
	m_arrowVolleyUITransform = m_arrowVolleyUI.getReferencedComponent();

	m_charge1Transform2D = m_charge1UI.getReferencedComponent();
	m_charge2Transform2D = m_charge2UI.getReferencedComponent();
	m_charge3Transform2D = m_charge3UI.getReferencedComponent();

	m_charge1Scale = m_chargedScale;
	m_charge2Scale = m_chargedScale;
	m_charge3Scale = m_chargedScale;

	hideChargedAttackUI();
	hideArrowVolleyUI();
}

void LyrielUI::showChargedAttackUI()
{
	if (!m_chargedAttackUITransform)
	{
		return;
	}

	GameObject* owner = m_chargedAttackUITransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, true);
}

void LyrielUI::updateChargedAttackUI(const Vector3& origin, const Vector3& aimDirection, float range)
{
	if (!m_chargedAttackUITransform)
	{
		return;
	}

	Vector3 flatDirection = aimDirection;
	flatDirection.y = 0.0f;

	if (flatDirection.LengthSquared() <= 0.0001f)
	{
		return;
	}

	flatDirection.Normalize();

	const float yawRad = std::atan2(flatDirection.x, flatDirection.z);
	const float targetYawDeg = yawRad * (180.0f / 3.14159265f);

	TransformAPI::setGlobalPosition(m_chargedAttackUITransform, origin);
	TransformAPI::setGlobalRotationEuler(m_chargedAttackUITransform, Vector3(0.0f, targetYawDeg, 0.0f));
	TransformAPI::setScale(m_chargedAttackUITransform, Vector3(1.0f, 1.0f, range));
}

void LyrielUI::hideChargedAttackUI()
{
	if (!m_chargedAttackUITransform)
	{
		return;
	}

	GameObject* owner = m_chargedAttackUITransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, false);
}

void LyrielUI::showArrowVolleyUI()
{
	if (!m_arrowVolleyUITransform)
	{
		return;
	}

	GameObject* owner = m_arrowVolleyUITransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, true);
}

void LyrielUI::updateArrowVolleyUI(const Vector3& origin, const Vector3& aimDirection)
{
	if (!m_arrowVolleyUITransform)
	{
		return;
	}

	Vector3 flatDirection = aimDirection;
	flatDirection.y = 0.0f;

	if (flatDirection.LengthSquared() <= 0.0001f)
	{
		return;
	}

	flatDirection.Normalize();

	const float yawRad = std::atan2(flatDirection.x, flatDirection.z);
	const float targetYawDeg = yawRad * (180.0f / 3.14159265f);

	TransformAPI::setGlobalPosition(m_arrowVolleyUITransform, origin);
	TransformAPI::setGlobalRotationEuler(m_arrowVolleyUITransform, Vector3(0.0f, targetYawDeg, 0.0f));
}

void LyrielUI::hideArrowVolleyUI()
{
	if (!m_arrowVolleyUITransform)
	{
		return;
	}

	GameObject* owner = m_arrowVolleyUITransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, false);
}

void LyrielUI::setupDashCharges(int maxCharges)
{
	m_charge1Scale = maxCharges >= 1 ? m_chargedScale : m_emptyScale;
	m_charge2Scale = maxCharges >= 2 ? m_chargedScale : m_emptyScale;
	m_charge3Scale = maxCharges >= 3 ? m_chargedScale : m_emptyScale;

	if (m_charge1Transform2D)
	{
		Transform2DAPI::setScale(m_charge1Transform2D, Vector2(m_charge1Scale, m_charge1Scale));
	}

	if (m_charge2Transform2D)
	{
		Transform2DAPI::setScale(m_charge2Transform2D, Vector2(m_charge2Scale, m_charge2Scale));
	}

	if (m_charge3Transform2D)
	{
		Transform2DAPI::setScale(m_charge3Transform2D, Vector2(m_charge3Scale, m_charge3Scale));
	}
}

void LyrielUI::updateDashChargesUI(int currentCharges, int maxCharges, float dt)
{
	const float target1 = currentCharges >= 1 ? m_chargedScale : m_emptyScale;
	const float target2 = currentCharges >= 2 ? m_chargedScale : m_emptyScale;
	const float target3 = currentCharges >= 3 ? m_chargedScale : m_emptyScale;

	if (maxCharges >= 1)
	{
		updateChargeVisual(m_charge1Transform2D, m_charge1Scale, target1, dt);
	}

	if (maxCharges >= 2)
	{
		updateChargeVisual(m_charge2Transform2D, m_charge2Scale, target2, dt);
	}

	if (maxCharges >= 3)
	{
		updateChargeVisual(m_charge3Transform2D, m_charge3Scale, target3, dt);
	}
}

void LyrielUI::updateChargeVisual(Transform2D* transform, float& currentScale, float targetScale, float dt)
{
	if (!transform)
	{
		return;
	}

	currentScale = MathAPI::moveTowards(currentScale, targetScale, m_uiScaleSpeed * dt);
	Transform2DAPI::setScale(transform, Vector2(currentScale, currentScale));
}

IMPLEMENT_SCRIPT(LyrielUI)