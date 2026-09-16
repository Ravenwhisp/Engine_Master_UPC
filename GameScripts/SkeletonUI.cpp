#include "pch.h"
#include "SkeletonUI.h"

namespace
{
	constexpr float RadiansToDegrees = 57.2957795f;
	constexpr float DegreesToRadians = 0.0174532925f;
}

IMPLEMENT_SCRIPT_FIELDS(SkeletonUI,
	FIELD_GROUP_COLLAPSE("Scimitar Attack",
		SERIALIZED_COMPONENT_REF(m_scimitarUICanvas, "Scimitar UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_scimitarUITelegraph, "Scimitar UI Telegraph", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_scimitarUIImpact, "Scimitar UI Impact", ComponentType::TRANSFORM2D),
		SERIALIZED_FLOAT(m_scimitarUIWidthMultiplier, "Width Multiplier", 0.1f, 5.0f, 0.01f),
		SERIALIZED_FLOAT(m_scimitarUILengthMultiplier, "Length Multiplier", 0.1f, 5.0f, 0.01f),
		SERIALIZED_FLOAT(m_scimitarUIForwardOffset, "Forward Offset", -5.0f, 5.0f, 0.01f),
		SERIALIZED_FLOAT(m_scimitarUISideOffset, "Side Offset", -5.0f, 5.0f, 0.01f),
		SERIALIZED_FLOAT(m_scimitarUIHeightOffset, "Height Offset", 0.0f, 1.0f, 0.01f),
		SERIALIZED_FLOAT(m_scimitarUITelegraphAlpha, "Telegraph Alpha", 0.0f, 1.0f, 0.01f),
		SERIALIZED_FLOAT(m_scimitarUIImpactAlpha, "Impact Alpha", 0.0f, 1.0f, 0.01f)
	)
)

SkeletonUI::SkeletonUI(GameObject* owner)
	: Script(owner)
{
}

void SkeletonUI::Start()
{
	m_scimitarUICanvasTransform = m_scimitarUICanvas.getReferencedComponent();
	m_scimitarUITelegraphTransform2D = m_scimitarUITelegraph.getReferencedComponent();
	m_scimitarUIImpactTransform2D = m_scimitarUIImpact.getReferencedComponent();

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

	if (!m_scimitarUICanvasTransform && ownerTransform)
	{
		m_scimitarUICanvasTransform = TransformAPI::findChildByName(ownerTransform, "SkeletonScimitarUI");
	}

	if (m_scimitarUICanvasTransform)
	{
		if (!m_scimitarUITelegraphTransform2D)
		{
			Transform* telegraphTransform = TransformAPI::findChildByName(m_scimitarUICanvasTransform, "Telegraph");

			if (telegraphTransform)
			{
				GameObject* telegraphObject = ComponentAPI::getOwner(telegraphTransform);
				m_scimitarUITelegraphTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(telegraphObject, ComponentType::TRANSFORM2D));
			}
		}

		if (!m_scimitarUIImpactTransform2D)
		{
			Transform* impactTransform = TransformAPI::findChildByName(m_scimitarUICanvasTransform, "Impact");

			if (impactTransform)
			{
				GameObject* impactObject = ComponentAPI::getOwner(impactTransform);
				m_scimitarUIImpactTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(impactObject, ComponentType::TRANSFORM2D));
			}
		}
	}

	hideScimitarUI();
}

void SkeletonUI::setupScimitarUI(float range, float halfAngleDegrees)
{
	m_currentScimitarRange = range;

	const float halfAngleRadians = halfAngleDegrees * DegreesToRadians;
	const float width = 2.0f * range * std::sin(halfAngleRadians);

	const Vector2 attackAreaScale(
		width * m_scimitarUIWidthMultiplier,
		range * m_scimitarUILengthMultiplier
	);

	if (m_scimitarUITelegraphTransform2D)
	{
		Transform2DAPI::setScale(m_scimitarUITelegraphTransform2D, attackAreaScale);
		Transform2DAPI::setAlpha(m_scimitarUITelegraphTransform2D, 0.0f);
	}

	if (m_scimitarUIImpactTransform2D)
	{
		Transform2DAPI::setScale(m_scimitarUIImpactTransform2D, attackAreaScale);
		Transform2DAPI::setAlpha(m_scimitarUIImpactTransform2D, 0.0f);
	}
}

void SkeletonUI::showScimitarTelegraph(const Vector3& origin, const Vector3& forwardDirection)
{
	if (!m_scimitarUICanvasTransform)
	{
		return;
	}

	GameObject* canvasObject = ComponentAPI::getOwner(m_scimitarUICanvasTransform);

	if (!canvasObject)
	{
		return;
	}

	updateScimitarUIPose(origin, forwardDirection);

	GameObjectAPI::setActive(canvasObject, true);

	if (m_scimitarUITelegraphTransform2D)
	{
		Transform2DAPI::setAlpha(m_scimitarUITelegraphTransform2D, m_scimitarUITelegraphAlpha);
	}

	if (m_scimitarUIImpactTransform2D)
	{
		Transform2DAPI::setAlpha(m_scimitarUIImpactTransform2D, 0.0f);
	}
}

void SkeletonUI::updateScimitarUIPose(const Vector3& origin, const Vector3& forwardDirection)
{
	if (!m_scimitarUICanvasTransform)
	{
		return;
	}

	Vector3 flatForward = forwardDirection;
	flatForward.y = 0.0f;

	if (flatForward.LengthSquared() < 0.0001f)
	{
		return;
	}

	flatForward.Normalize();

	Vector3 right(-flatForward.z, 0.0f, flatForward.x);
	right.Normalize();

	Vector3 adjustedPosition = origin;
	adjustedPosition += flatForward * (m_currentScimitarRange * 0.5f);
	adjustedPosition += flatForward * m_scimitarUIForwardOffset;
	adjustedPosition += right * m_scimitarUISideOffset;
	adjustedPosition.y += m_scimitarUIHeightOffset;

	const float yawDegrees = std::atan2(flatForward.x, flatForward.z) * RadiansToDegrees;

	TransformAPI::setGlobalPosition(m_scimitarUICanvasTransform, adjustedPosition);
	TransformAPI::setGlobalRotationEuler(m_scimitarUICanvasTransform, Vector3(90.0f, yawDegrees, 0.0f));
}

void SkeletonUI::showScimitarImpact()
{
	if (m_scimitarUITelegraphTransform2D)
	{
		Transform2DAPI::setAlpha(m_scimitarUITelegraphTransform2D, 0.0f);
	}

	if (m_scimitarUIImpactTransform2D)
	{
		Transform2DAPI::setAlpha(m_scimitarUIImpactTransform2D, m_scimitarUIImpactAlpha);
	}
}

void SkeletonUI::hideScimitarUI()
{
	if (!m_scimitarUICanvasTransform)
	{
		return;
	}

	GameObject* owner = m_scimitarUICanvasTransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, false);
}

IMPLEMENT_SCRIPT(SkeletonUI)