#include "pch.h"
#include "LyrielUI.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(LyrielUI, CharacterUI,
	FIELD_GROUP_LABEL("World-space Attack UI"),
	SERIALIZED_FLOAT(m_attackUIHeightOffset, "Height Offset", 0.0f, 1.0f, 0.01f),

	FIELD_GROUP_LABEL("Basic Attack Aim"),
	SERIALIZED_COMPONENT_REF(m_basicAttackUI, "Basic Attack Aim UI", ComponentType::TRANSFORM),
	SERIALIZED_FLOAT(m_basicAttackYawOffset, "Basic Attack Yaw Offset (deg)", -360.0f, 360.0f, 5.0f),

	FIELD_GROUP_LABEL("Charged Attack"),
	SERIALIZED_COMPONENT_REF(m_chargedAttackUI, "Charged Attack UI", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_chargedHUDControl, "Charged Attack HUD Control", ComponentType::TRANSFORM2D),

	FIELD_GROUP_LABEL("Arrow Volley"),
	SERIALIZED_COMPONENT_REF(m_arrowVolleyUI, "Arrow Volley UI", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_arrowVolleyHUDControl, "Arrow Volley HUD Control", ComponentType::TRANSFORM2D),

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

	m_basicAttackUITransform = m_basicAttackUI.getReferencedComponent();
	m_chargedAttackUITransform = m_chargedAttackUI.getReferencedComponent();
	m_arrowVolleyUITransform = m_arrowVolleyUI.getReferencedComponent();

	m_chargedHUDControlTransform2D = m_chargedHUDControl.getReferencedComponent();
	m_arrowVolleyHUDControlTransform2D = m_arrowVolleyHUDControl.getReferencedComponent();

	m_charge1Transform2D = m_charge1UI.getReferencedComponent();
	m_charge2Transform2D = m_charge2UI.getReferencedComponent();
	m_charge3Transform2D = m_charge3UI.getReferencedComponent();

	m_charge1Scale = m_chargedScale;
	m_charge2Scale = m_chargedScale;
	m_charge3Scale = m_chargedScale;

	hideBasicAttackUI();
	hideChargedAttackUI();
	hideArrowVolleyUI();
}

void LyrielUI::showBasicAttackUI()
{
	if (!m_basicAttackUITransform)
	{
		return;
	}

	GameObject* owner = m_basicAttackUITransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, true);
}

void LyrielUI::hideBasicAttackUI()
{
	if (!m_basicAttackUITransform)
	{
		return;
	}

	GameObject* owner = m_basicAttackUITransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, false);
}

void LyrielUI::updateBasicAttackUI(const Vector3& origin, const Vector3& aimDirection)
{
	if (!m_basicAttackUITransform)
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
	const float targetYawDeg = yawRad * (180.0f / 3.14159265f) + m_basicAttackYawOffset;

	Vector3 uiPosition = origin;
	uiPosition.y += m_attackUIHeightOffset;

	TransformAPI::setGlobalPosition(m_basicAttackUITransform, uiPosition);
	TransformAPI::setGlobalRotationEuler(m_basicAttackUITransform, Vector3(0.0f, targetYawDeg, 0.0f));
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

	Vector3 uiPosition = origin;
	uiPosition.y += m_attackUIHeightOffset;

	TransformAPI::setGlobalPosition(m_chargedAttackUITransform, uiPosition);
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

	Vector3 uiPosition = origin;
	uiPosition.y += m_attackUIHeightOffset;

	TransformAPI::setGlobalPosition(m_arrowVolleyUITransform, uiPosition);
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

void LyrielUI::updateDashChargesUI(int currentCharges, int maxCharges, float dt)
{
	updateChargeVisual(m_charge1Transform2D, m_charge1Scale, currentCharges >= 1 && maxCharges >= 1, dt);
	updateChargeVisual(m_charge2Transform2D, m_charge2Scale, currentCharges >= 2 && maxCharges >= 2, dt);
	updateChargeVisual(m_charge3Transform2D, m_charge3Scale, currentCharges >= 3 && maxCharges >= 3, dt);
}

void LyrielUI::updateChargeVisual(Transform2D* transform, float& currentScale, bool visible, float dt)
{
	if (!transform)
	{
		return;
	}

	const float targetScale = visible ? m_chargedScale : m_emptyScale;
	currentScale = MathAPI::moveTowards(currentScale, targetScale, m_uiScaleSpeed * dt);

	Transform2DAPI::setScale(transform, Vector2(currentScale, currentScale));
}

IMPLEMENT_SCRIPT(LyrielUI)
