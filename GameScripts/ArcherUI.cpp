#include "pch.h"
#include "ArcherUI.h"

IMPLEMENT_SCRIPT_FIELDS(ArcherUI,
	FIELD_GROUP_COLLAPSE("Arrow Barrage",
		SERIALIZED_COMPONENT_REF(m_arrowBarrageUICanvas, "Arrow Barrage UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_arrowBarrageUIContainer, "Arrow Barrage UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_arrowBarrageUIGlow, "Arrow Barrage UI Glow", ComponentType::TRANSFORM2D)
	)
)

ArcherUI::ArcherUI(GameObject* owner)
	: Script(owner)
{
}

void ArcherUI::Start()
{
	m_arrowBarrageUICanvasTransform = m_arrowBarrageUICanvas.getReferencedComponent();
	m_arrowBarrageUIContainerTransform2D = m_arrowBarrageUIContainer.getReferencedComponent();
	m_arrowBarrageUIGlowTransform2D = m_arrowBarrageUIGlow.getReferencedComponent();

	if (!m_arrowBarrageUICanvasTransform || !m_arrowBarrageUIContainerTransform2D || !m_arrowBarrageUIGlowTransform2D)
	{
		Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
		m_arrowBarrageUICanvasTransform = TransformAPI::findChildByName(ownerTransform, "Arrow Barrage UI");
		if (m_arrowBarrageUICanvasTransform)
		{
			GameObject* arrowBarrageUIObject = ComponentAPI::getOwner(m_arrowBarrageUICanvasTransform);
			if (!m_arrowBarrageUIContainerTransform2D)
			{
				m_arrowBarrageUIContainerTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(arrowBarrageUIObject, ComponentType::TRANSFORM2D));
			}
			if (!m_arrowBarrageUIGlowTransform2D)
			{
				m_arrowBarrageUIGlowTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(arrowBarrageUIObject, ComponentType::TRANSFORM2D));
			}
		}
	}

	hideArrowBarrageUI();
}

void ArcherUI::setupArrowBarrageUI(float radius)
{
	if (!m_arrowBarrageUIContainerTransform2D || !m_arrowBarrageUIGlowTransform2D)
	{
		return;
	}

	Transform2DAPI::setAlpha(m_arrowBarrageUIContainerTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_arrowBarrageUIGlowTransform2D, 0.0f);
	Transform2DAPI::setScale(m_arrowBarrageUIContainerTransform2D, Vector2(radius, radius));
}

void ArcherUI::updateArrowBarrageUI(float stateTimer, const Vector3& impactPosition, float throwTime, float landDelay, float totalDuration)
{
	if (!m_arrowBarrageUICanvasTransform || !m_arrowBarrageUIContainerTransform2D || !m_arrowBarrageUIGlowTransform2D)
	{
		return;
	}

	if (stateTimer < throwTime)
	{
		return;
	}

	const float impactTime = throwTime + landDelay;

	if (stateTimer < impactTime)
	{
		GameObjectAPI::setActive(m_arrowBarrageUICanvasTransform->getOwner(), true);
		TransformAPI::setGlobalPosition(m_arrowBarrageUICanvasTransform, impactPosition);

		const float t = std::clamp((stateTimer - throwTime) / landDelay, 0.0f, 1.0f);
		Transform2DAPI::setAlpha(m_arrowBarrageUIContainerTransform2D, t);
		return;
	}

	Transform2DAPI::setAlpha(m_arrowBarrageUIGlowTransform2D, 1.0f);

	const float fadeDuration = totalDuration - impactTime;
	const float t = fadeDuration > 0.001f ? 1.0f - ((stateTimer - impactTime) / fadeDuration) : 0.0f;
	Transform2DAPI::setAlpha(m_arrowBarrageUIContainerTransform2D, std::clamp(t, 0.0f, 1.0f));
}

void ArcherUI::hideArrowBarrageUI()
{
	if (!m_arrowBarrageUICanvasTransform)
	{
		return;
	}

	GameObject* owner = m_arrowBarrageUICanvasTransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, false);
}

IMPLEMENT_SCRIPT(ArcherUI)