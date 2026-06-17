#include "pch.h"
#include "ArthurUI.h"

IMPLEMENT_SCRIPT_FIELDS(ArthurUI,
	FIELD_GROUP_COLLAPSE("Health Bar",
		SERIALIZED_COMPONENT_REF(m_healthBarCanvas, "Health Bar Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_healthBarContainer, "Health Bar Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_healthBarPhase2, "Health Bar Phase 2", ComponentType::TRANSFORM2D),
		SERIALIZED_FLOAT(m_healthBarDuration, "Health Bar Duration", 0.0f, 10.0f, 0.1f)
	),

	FIELD_GROUP_COLLAPSE("Heavy Swipe", 
		SERIALIZED_COMPONENT_REF(m_heavySwipeUICanvas, "Heavy Swipe UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_heavySwipeUIContainer, "Heavy Swipe UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_heavySwipeUIBackground, "Heavy Swipe UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_heavySwipeUIBorder, "Heavy Swipe UI Border", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_heavySwipeUIGlow, "Heavy Swipe UI Glow", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_heavySwipeUIRightClaw, "Heavy Swipe UI Right Claw", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_heavySwipeUILeftClaw, "Heavy Swipe UI Left Claw", ComponentType::TRANSFORM2D)
	),

	FIELD_GROUP_COLLAPSE("Side Sweep",
		SERIALIZED_COMPONENT_REF(m_sideSweepUICanvas, "Side Sweep UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_sideSweepUIContainer, "Side Sweep UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_sideSweepUIBackground, "Side Sweep UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_sideSweepUIShadow, "Side Sweep UI Shadow", ComponentType::TRANSFORM2D)
	),

	FIELD_GROUP_COLLAPSE("Charging Slam",
		SERIALIZED_COMPONENT_REF(m_chargingSlamUICanvas, "Charging Slam UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_chargingSlamUIContainer, "Charging Slam UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_chargingSlamUIBackground, "Charging Slam UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_chargingSlamUIBorders, "Charging Slam UI Borders", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_chargingSlamUIShadow, "Charging Slam UI Shadow", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_chargingSlamUISpikes, "Charging Slam UI Spikes", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_chargingSlamUIBordersSlider, "Charging Slam UI Borders Slider", ComponentType::UISLIDER),
		SERIALIZED_COMPONENT_REF(m_chargingSlamUIShadowSlider, "Charging Slam UI Shadow Slider", ComponentType::UISLIDER),
		SERIALIZED_COMPONENT_REF(m_chargingSlamImpactUICanvas, "Charging Slam Impact UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_chargingSlamImpactUIContainer, "Charging Slam Impact UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_chargingSlamImpactUICenter, "Charging Slam Impact UI Center", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_chargingSlamImpactUIGlow, "Charging Slam Impact UI Glow", ComponentType::TRANSFORM2D)
	),

	FIELD_GROUP_COLLAPSE("Earth Hammer",
		SERIALIZED_COMPONENT_REF(m_earthHammerUICanvas, "Earth Hammer UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_earthHammerUIContainer, "Earth Hammer UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_earthHammerUIInner, "Earth Hammer UI Inner", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_earthHammerUISpikes, "Earth Hammer UI Spikes", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_earthHammerUIGlow, "Earth Hammer UI Glow", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_earthHammerUIRing, "Earth Hammer UI Ring", ComponentType::TRANSFORM2D)
	)
)

ArthurUI::ArthurUI(GameObject* owner)
	: Script(owner)
{
}

void ArthurUI::Start()
{
	setupHealthUI();

	m_heavySwipeUICanvasTransform = m_heavySwipeUICanvas.getReferencedComponent();
	m_heavySwipeUIContainerTransform2D = m_heavySwipeUIContainer.getReferencedComponent();
	m_heavySwipeUIBackgroundTransform2D = m_heavySwipeUIBackground.getReferencedComponent();
	m_heavySwipeUIBorderTransform2D = m_heavySwipeUIBorder.getReferencedComponent();
	m_heavySwipeUIGlowTransform2D = m_heavySwipeUIGlow.getReferencedComponent();
	m_heavySwipeUIRightClawTransform2D = m_heavySwipeUIRightClaw.getReferencedComponent();
	m_heavySwipeUILeftClawTransform2D = m_heavySwipeUILeftClaw.getReferencedComponent();

	hideHeavySwipeUI();

	m_sideSweepUICanvasTransform = m_sideSweepUICanvas.getReferencedComponent();
	m_sideSweepUIContainerTransform2D = m_sideSweepUIContainer.getReferencedComponent();
	m_sideSweepUIBackgroundTransform2D = m_sideSweepUIBackground.getReferencedComponent();
	m_sideSweepUIShadowTransform2D = m_sideSweepUIShadow.getReferencedComponent();

	hideSideSweepUI();

	m_chargingSlamUICanvasTransform = m_chargingSlamUICanvas.getReferencedComponent();
	m_chargingSlamUIContainerTransform2D = m_chargingSlamUIContainer.getReferencedComponent();
	m_chargingSlamUIBackgroundTransform2D = m_chargingSlamUIBackground.getReferencedComponent();
	m_chargingSlamUIBordersTransform2D = m_chargingSlamUIBorders.getReferencedComponent();
	m_chargingSlamUIShadowTransform2D = m_chargingSlamUIShadow.getReferencedComponent();
	m_chargingSlamUISpikesTransform2D = m_chargingSlamUISpikes.getReferencedComponent();
	m_chargingSlamUIBordersSliderComponent = m_chargingSlamUIBordersSlider.getReferencedComponent();
	m_chargingSlamUIShadowSliderComponent = m_chargingSlamUIShadowSlider.getReferencedComponent();

	m_chargingSlamImpactUICanvasTransform = m_chargingSlamImpactUICanvas.getReferencedComponent();
	m_chargingSlamImpactUIContainerTransform2D = m_chargingSlamImpactUIContainer.getReferencedComponent();
	m_chargingSlamImpactUICenterTransform2D = m_chargingSlamImpactUICenter.getReferencedComponent();
	m_chargingSlamImpactUIGlowTransform2D = m_chargingSlamImpactUIGlow.getReferencedComponent();

	hideChargingSlamUI();

	m_earthHammerUICanvasTransform = m_earthHammerUICanvas.getReferencedComponent();
	m_earthHammerUIContainerTransform2D = m_earthHammerUIContainer.getReferencedComponent();
	m_earthHammerUIInnerTransform2D = m_earthHammerUIInner.getReferencedComponent();
	m_earthHammerUISpikesTransform2D = m_earthHammerUISpikes.getReferencedComponent();
	m_earthHammerUIGlowTransform2D = m_earthHammerUIGlow.getReferencedComponent();
	m_earthHammerUIRingTransform2D = m_earthHammerUIRing.getReferencedComponent();

	hideEarthHammerUI();
}

void ArthurUI::Update()
{
	updateHealthUI();
}

void ArthurUI::setupHealthUI()
{
	m_healthBarCanvasTransform = m_healthBarCanvas.getReferencedComponent();
	m_healthBarContainerTransform2D = m_healthBarContainer.getReferencedComponent();
	m_healthBarPhase2Transform2D = m_healthBarPhase2.getReferencedComponent();

	if (!m_healthBarCanvasTransform)
	{
		Debug::warn("[ArthurUI] Health Bar Canvas reference is missing.");
		return;
	}

	GameObject* canvasOwner = m_healthBarCanvasTransform->getOwner();
	if (!canvasOwner)
	{
		Debug::warn("[ArthurUI] Health Bar Canvas owner is null.");
		return;
	}

	GameObjectAPI::setActive(canvasOwner, false);

	if (m_healthBarContainerTransform2D)
	{
		Transform2DAPI::setAlpha(m_healthBarContainerTransform2D, 0.0f);
	}

	if (m_healthBarPhase2Transform2D)
	{
		Transform2DAPI::setAlpha(m_healthBarPhase2Transform2D, 0.0f);
	}
}

void ArthurUI::updateHealthUI()
{
	if (!m_healthBarCanvasTransform || !m_healthBarContainerTransform2D)
	{
		return;
	}

	GameObject* canvasOwner = m_healthBarCanvasTransform->getOwner();
	if (!canvasOwner)
	{
		return;
	}

	if (m_healthBarTimer > 0.0f)
	{
		m_healthBarTimer -= Time::getDeltaTime();

		const float duration = m_healthBarDuration > 0.0f ? m_healthBarDuration : 0.0001f;
		const float t = std::clamp(m_healthBarTimer / duration, 0.0f, 1.0f);

		const float size = -Transform2DAPI::getBaseSize(m_healthBarContainerTransform2D).y * 0.5f;
		const float position = (m_healthBarVisible ? t : 1.0f - t) * size;
		const float alpha = m_healthBarVisible ? 1.0f - t : t;

		Transform2DAPI::setPosition(m_healthBarContainerTransform2D, Vector2(0.0f, position));
		Transform2DAPI::setAlpha(m_healthBarContainerTransform2D, alpha);
	}

	if (m_healthBarTimer <= 0.0f)
	{
		GameObjectAPI::setActive(canvasOwner, m_healthBarVisible);
		Transform2DAPI::setAlpha(m_healthBarContainerTransform2D, m_healthBarVisible ? 1.0f : 0.0f);
	}

	if (m_healthBarPhase2Transform2D && m_healthBarPhase2Timer > 0.0f)
	{
		m_healthBarPhase2Timer -= Time::getDeltaTime();

		const float duration = m_healthBarDuration > 0.0f ? m_healthBarDuration : 0.0001f;
		const float t = 1.0f - std::clamp(m_healthBarPhase2Timer / duration, 0.0f, 1.0f);

		Transform2DAPI::setAlpha(m_healthBarPhase2Transform2D, t);
	}
}

void ArthurUI::showHealthUI(bool show)
{
	if (!m_healthBarCanvasTransform)
	{
		return;
	}

	GameObject* canvasOwner = m_healthBarCanvasTransform->getOwner();
	if (!canvasOwner)
	{
		return;
	}

	m_healthBarVisible = show;
	m_healthBarTimer = m_healthBarDuration;

	GameObjectAPI::setActive(canvasOwner, true);
}

void ArthurUI::updateHealthUIPhase()
{
	if (!m_healthBarPhase2Transform2D)
	{
		return;
	}

	m_healthBarPhase2Visible = true;
	m_healthBarPhase2Timer = m_healthBarDuration;
}

void ArthurUI::setupHeavySwipeUI()
{
	if (!m_heavySwipeUICanvasTransform ||
		!m_heavySwipeUIContainerTransform2D ||
		!m_heavySwipeUIBackgroundTransform2D ||
		!m_heavySwipeUIBorderTransform2D ||
		!m_heavySwipeUIGlowTransform2D ||
		!m_heavySwipeUIRightClawTransform2D ||
		!m_heavySwipeUILeftClawTransform2D)
	{
		return;
	}

	GameObjectAPI::setActive(m_heavySwipeUICanvasTransform->getOwner(), true);

	Transform2DAPI::setAlpha(m_heavySwipeUIContainerTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_heavySwipeUIBackgroundTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_heavySwipeUIBorderTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_heavySwipeUIGlowTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_heavySwipeUIRightClawTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_heavySwipeUILeftClawTransform2D, 0.0f);
}

void ArthurUI::updateHeavySwipeUI(float stateTimer, bool isPhase2, float hit1Time, float hit2Time, float hit3Time, float hit4Time, float totalDuration, float heavySwipeRange)
{
	if (!m_heavySwipeUIContainerTransform2D || !m_heavySwipeUIBackgroundTransform2D || !m_heavySwipeUIBorderTransform2D || !m_heavySwipeUIGlowTransform2D || !m_heavySwipeUIRightClawTransform2D || !m_heavySwipeUILeftClawTransform2D)
	{
		return;
	}

	if (stateTimer < hit1Time)
	{
		const float t = std::clamp(stateTimer / hit1Time, 0.0f, 1.0f);
		Transform2DAPI::setAlpha(m_heavySwipeUIContainerTransform2D, t);
	}
	else if (stateTimer < hit2Time)
	{
		const float t = (stateTimer - hit1Time) / (hit2Time - hit1Time);
		applyHeavySwipeHitEffects(t, m_heavySwipeUIGlowTransform2D, m_heavySwipeUIBorderTransform2D, m_heavySwipeUILeftClawTransform2D, heavySwipeRange);
	}
	else if (stateTimer < hit3Time)
	{
		const float t = (stateTimer - hit2Time) / (hit3Time - hit2Time);
		applyHeavySwipeHitEffects(t, m_heavySwipeUIGlowTransform2D, m_heavySwipeUIBorderTransform2D, m_heavySwipeUIRightClawTransform2D, heavySwipeRange);
	}
	else if (isPhase2 && stateTimer < hit4Time)
	{
		const float t = (stateTimer - hit3Time) / (hit4Time - hit3Time);
		applyHeavySwipeHitEffects(t, m_heavySwipeUIGlowTransform2D, m_heavySwipeUIBorderTransform2D, m_heavySwipeUILeftClawTransform2D, heavySwipeRange);
	}
	else if (stateTimer <= totalDuration)
	{
		const float lastHitTime = isPhase2 ? hit4Time : hit3Time;
		const float t = (stateTimer - lastHitTime) / (totalDuration - lastHitTime);

		applyHeavySwipeHitEffects(t, m_heavySwipeUIGlowTransform2D, m_heavySwipeUIBorderTransform2D, isPhase2 ? m_heavySwipeUIRightClawTransform2D : m_heavySwipeUILeftClawTransform2D,heavySwipeRange);

		const float alpha = MathAPI::moveTowards(t, 1.0f, 0.3f);
		Transform2DAPI::setAlpha(m_heavySwipeUIContainerTransform2D, alpha);
	}
}

void ArthurUI::applyHeavySwipeHitEffects(float t, Transform2D* glow, Transform2D* border, Transform2D* claw, float heavySwipeRange)
{
	if (!glow || !border || !claw)
	{
		return;
	}

	const float glowAlpha = MathAPI::pingPong(t);
	Transform2DAPI::setAlpha(glow, glowAlpha);

	const float borderScale = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, std::clamp(t, 0.1f, 1.0f)) * heavySwipeRange;

	Transform2DAPI::setScale(border, Vector2(borderScale, borderScale));

	const float anchorValue = MathAPI::lerp(0.5f, 1.0f, t);

	Transform2DAPI::setAlpha(claw, t);
	Transform2DAPI::setAnchorMin(claw, Vector2(0.5f, anchorValue));
}

void ArthurUI::hideHeavySwipeUI()
{
	if (!m_heavySwipeUICanvasTransform)
	{
		return;
	}

	GameObject* owner = m_heavySwipeUICanvasTransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, false);
}

void ArthurUI::setupSideSweepUI(int sweepSide)
{
	if (!m_sideSweepUICanvasTransform || !m_sideSweepUIContainerTransform2D || !m_sideSweepUIBackgroundTransform2D || !m_sideSweepUIShadowTransform2D)
	{
		return;
	}

	GameObjectAPI::setActive(m_sideSweepUICanvasTransform->getOwner(), true);

	if (sweepSide == -1)
	{
		TransformAPI::setRotationEuler(m_sideSweepUICanvasTransform, Vector3(90.0f, 0.0f, -90.0f));
	}
	else
	{
		TransformAPI::setRotationEuler(m_sideSweepUICanvasTransform, Vector3(90.0f, 0.0f, 90.0f));
	}

	Transform2DAPI::setAlpha(m_sideSweepUIBackgroundTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_sideSweepUIShadowTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_sideSweepUIContainerTransform2D, 1.0f);
}

void ArthurUI::updateSideSweepUI(float stateTimer, float hitTime, float totalDuration)
{
	if (!m_sideSweepUIContainerTransform2D || !m_sideSweepUIBackgroundTransform2D || !m_sideSweepUIShadowTransform2D)
	{
		return;
	}

	if (stateTimer < hitTime)
	{
		const float t = std::clamp(stateTimer / hitTime, 0.0f, 1.0f);
		Transform2DAPI::setAlpha(m_sideSweepUIBackgroundTransform2D, t);
	}
	else
	{
		Transform2DAPI::setAlpha(m_sideSweepUIBackgroundTransform2D, 1.0f);

		const float t = std::clamp((stateTimer - hitTime) / (totalDuration - hitTime), 0.0f, 1.0f);
		const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, t);

		Transform2DAPI::setAlpha(m_sideSweepUIShadowTransform2D, easedT);
		Transform2DAPI::setAlpha(m_sideSweepUIContainerTransform2D, 1.0f - easedT);
	}
}

void ArthurUI::hideSideSweepUI()
{
	if (!m_sideSweepUICanvasTransform)
	{
		return;
	}

	GameObject* owner = m_sideSweepUICanvasTransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, false);
}

void ArthurUI::setupChargingSlamUI(const Vector3& startPosition, const Vector3& lockedTargetPosition, const Vector3& dashDirection)
{
	if (!m_chargingSlamUICanvasTransform || !m_chargingSlamUIContainerTransform2D || !m_chargingSlamUIBordersTransform2D || !m_chargingSlamUIShadowTransform2D || !m_chargingSlamUIBackgroundTransform2D || !m_chargingSlamUISpikesTransform2D || !m_chargingSlamUIBordersSliderComponent || !m_chargingSlamUIShadowSliderComponent)
	{
		return;
	}

	m_isChargingSlamUIFading = false;
	m_chargingSlamUIFadeOutTimer = 0.0f;

	GameObjectAPI::setActive(m_chargingSlamUICanvasTransform->getOwner(), true);

	SliderAPI::setFillAmount(m_chargingSlamUIBordersSliderComponent, 0.0f);
	SliderAPI::setFillAmount(m_chargingSlamUIShadowSliderComponent, 0.0f);
	Transform2DAPI::setAlpha(m_chargingSlamUIBackgroundTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_chargingSlamUIShadowTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_chargingSlamUISpikesTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_chargingSlamUIContainerTransform2D, 1.0f);
	Transform2DAPI::setPivot(m_chargingSlamUIContainerTransform2D, Vector2(0.5f, 1.0f));
	Transform2DAPI::setAnchorMin(m_chargingSlamUIContainerTransform2D, Vector2(0.5f, 1.0f));

	const float distance = Vector3::Distance(startPosition, lockedTargetPosition);
	const float baseWidth = Transform2DAPI::getBaseSize(m_chargingSlamUIContainerTransform2D).x;
	Transform2DAPI::setBaseSize(m_chargingSlamUIContainerTransform2D, Vector2(baseWidth, distance * 100.0f));

	m_isChargingSlamImpactUIPlaying = false;
	m_chargingSlamImpactUITimer = 0.0f;
	m_isChargingSlamImpactUIFading = false;
	m_chargingSlamImpactUIFadeTimer = 0.0f;

	if (!m_chargingSlamImpactUICanvasTransform || !m_chargingSlamImpactUIContainerTransform2D || !m_chargingSlamImpactUICenterTransform2D || !m_chargingSlamImpactUIGlowTransform2D)
	{
		return;
	}

	GameObjectAPI::setActive(m_chargingSlamImpactUICanvasTransform->getOwner(), true);
	TransformAPI::setPosition(m_chargingSlamImpactUICanvasTransform, Vector3(lockedTargetPosition.x, lockedTargetPosition.y, lockedTargetPosition.z));
	TransformAPI::setRotationEuler(m_chargingSlamImpactUICanvasTransform, Vector3(90.0f, 0.0f, atan2(dashDirection.z, dashDirection.x) * 180.0f / 3.14159265f - 90.0f));

	Transform2DAPI::setAlpha(m_chargingSlamImpactUIContainerTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_chargingSlamImpactUICenterTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_chargingSlamImpactUIGlowTransform2D, 0.0f);
}

void ArthurUI::updateChargingSlamUI(float stateTimer, bool isPhase2, bool hasStartedDash, bool hasAppliedImpact, const Vector3& startPosition, const Vector3& lockedTargetPosition, float chargeDuration)
{
	if (!m_chargingSlamUIContainerTransform2D || !m_chargingSlamUIBordersTransform2D || !m_chargingSlamUIShadowTransform2D || !m_chargingSlamUIBackgroundTransform2D || !m_chargingSlamUISpikesTransform2D || !m_chargingSlamUIBordersSliderComponent || !m_chargingSlamUIShadowSliderComponent || !m_chargingSlamImpactUIContainerTransform2D || !m_chargingSlamImpactUICenterTransform2D || !m_chargingSlamImpactUIGlowTransform2D)
	{
		return;
	}

	const float deltaTime = Time::getDeltaTime();

	if (!hasStartedDash)
	{
		const float t = std::clamp(stateTimer / chargeDuration, 0.0f, 1.0f);

		Transform2DAPI::setAlpha(m_chargingSlamUIShadowTransform2D, t);
		SliderAPI::setFillAmount(m_chargingSlamUIShadowSliderComponent, t);
		Transform2DAPI::setAlpha(m_chargingSlamUISpikesTransform2D, t);

		const float bordersFill = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);
		SliderAPI::setFillAmount(m_chargingSlamUIBordersSliderComponent, bordersFill);

		const float backgroundAlpha = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);
		Transform2DAPI::setAlpha(m_chargingSlamUIBackgroundTransform2D, backgroundAlpha);

		SliderAPI::setFillOrigin(m_chargingSlamUIBordersSliderComponent, FillOrigin::VerticalTop);
		SliderAPI::setFillOrigin(m_chargingSlamUIShadowSliderComponent, FillOrigin::VerticalTop);
	}

	if (hasStartedDash && !m_isChargingSlamUIFading)
	{
		SliderAPI::setFillOrigin(m_chargingSlamUIBordersSliderComponent, FillOrigin::VerticalBottom);
		SliderAPI::setFillOrigin(m_chargingSlamUIShadowSliderComponent, FillOrigin::VerticalBottom);

		Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

		if (!ownerTransform)
		{
			return;
		}

		Vector3 currentPosition = TransformAPI::getGlobalPosition(ownerTransform);

		const float totalDistance = Vector3::Distance(startPosition, lockedTargetPosition);
		const float remainingDistance = Vector3::Distance(currentPosition, lockedTargetPosition);

		float dashT = 1.0f;

		if (totalDistance > 0.001f)
		{
			dashT = remainingDistance / totalDistance;
		}

		dashT = std::clamp(dashT, 0.0f, 1.0f);

		SliderAPI::setFillAmount(m_chargingSlamUIBordersSliderComponent, dashT);
		SliderAPI::setFillAmount(m_chargingSlamUIShadowSliderComponent, dashT);
		Transform2DAPI::setPivot(m_chargingSlamUIContainerTransform2D, Vector2(0.5f, dashT));
		Transform2DAPI::setAnchorMin(m_chargingSlamUIContainerTransform2D, Vector2(0.5f, dashT));
	}

	if (!hasAppliedImpact)
	{
		float t = std::clamp(stateTimer / chargeDuration, 0.0f, 1.0f);
		t = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);
		Transform2DAPI::setAlpha(m_chargingSlamImpactUIContainerTransform2D, t);
	}

	if (m_isChargingSlamImpactUIPlaying)
	{
		m_chargingSlamImpactUITimer += deltaTime;

		const float duration = 0.45f;
		const float t = std::clamp(m_chargingSlamImpactUITimer / duration, 0.0f, 1.0f);

		const float centerAlpha = 1.0f - MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);
		Transform2DAPI::setAlpha(m_chargingSlamImpactUICenterTransform2D, centerAlpha);

		float glowAlpha = MathAPI::pingPong(t);
		glowAlpha = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, glowAlpha);
		Transform2DAPI::setAlpha(m_chargingSlamImpactUIGlowTransform2D, glowAlpha);

		if (t >= 1.0f)
		{
			m_isChargingSlamImpactUIPlaying = false;
			m_isChargingSlamImpactUIFading = true;
			m_chargingSlamImpactUIFadeTimer = 0.0f;
		}
	}

	if (m_isChargingSlamUIFading)
	{
		const float fadeDuration = 0.35f;

		m_chargingSlamUIFadeOutTimer += deltaTime;

		float t = std::clamp(m_chargingSlamUIFadeOutTimer / fadeDuration, 0.0f, 1.0f);
		t = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);

		const float alpha = 1.0f - t;
		Transform2DAPI::setAlpha(m_chargingSlamUIContainerTransform2D, alpha);

		if (t >= 1.0f && m_chargingSlamUICanvasTransform)
		{
			GameObjectAPI::setActive(m_chargingSlamUICanvasTransform->getOwner(), false);
		}
	}

	if (m_isChargingSlamImpactUIFading)
	{
		const float fadeDuration = 0.35f;

		m_chargingSlamImpactUIFadeTimer += deltaTime;

		float t = std::clamp(m_chargingSlamImpactUIFadeTimer / fadeDuration, 0.0f, 1.0f);
		t = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);

		const float alpha = 1.0f - t;
		Transform2DAPI::setAlpha(m_chargingSlamImpactUIContainerTransform2D, alpha);

		if (t >= 1.0f && m_chargingSlamImpactUICanvasTransform)
		{
			GameObjectAPI::setActive(m_chargingSlamImpactUICanvasTransform->getOwner(), false);
		}
	}
}

void ArthurUI::startChargingSlamImpactUI()
{
	m_isChargingSlamUIFading = true;
	m_chargingSlamUIFadeOutTimer = 0.0f;

	m_isChargingSlamImpactUIPlaying = true;
	m_chargingSlamImpactUITimer = 0.0f;
}

void ArthurUI::hideChargingSlamUI()
{
	if (m_chargingSlamUICanvasTransform)
	{
		GameObjectAPI::setActive(m_chargingSlamUICanvasTransform->getOwner(), false);
	}

	if (m_chargingSlamImpactUICanvasTransform)
	{
		GameObjectAPI::setActive(m_chargingSlamImpactUICanvasTransform->getOwner(), false);
	}
}

void ArthurUI::setupEarthHammerUI()
{
	if (!m_earthHammerUICanvasTransform || !m_earthHammerUIContainerTransform2D || !m_earthHammerUIRingTransform2D || !m_earthHammerUIInnerTransform2D || !m_earthHammerUISpikesTransform2D || !m_earthHammerUIGlowTransform2D)
	{
		return;
	}

	m_earthHammerHasStartedImpactUI = false;
	m_earthHammerImpactUITimer = 0.0f;
	m_earthHammerInnerScale = 0.1f;

	GameObjectAPI::setActive(m_earthHammerUICanvasTransform->getOwner(), true);

	Transform2DAPI::setAlpha(m_earthHammerUIRingTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_earthHammerUIInnerTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_earthHammerUISpikesTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_earthHammerUIGlowTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_earthHammerUIContainerTransform2D, 0.0f);
}

void ArthurUI::updateEarthHammerUI(float stateTimer, bool hasAppliedImpact, float hitTime, float impactDuration)
{
	if (!m_earthHammerUIContainerTransform2D || !m_earthHammerUIRingTransform2D || !m_earthHammerUIInnerTransform2D || !m_earthHammerUISpikesTransform2D || !m_earthHammerUIGlowTransform2D)
	{
		return;
	}

	if (!hasAppliedImpact)
	{
		const float t = std::clamp(stateTimer / hitTime, 0.0f, 1.0f);

		const float ringAlpha = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);
		Transform2DAPI::setAlpha(m_earthHammerUIContainerTransform2D, ringAlpha);

		m_earthHammerInnerScale = 0.1f + (t * 0.9f);
		Transform2DAPI::setScale(m_earthHammerUIInnerTransform2D, Vector2(m_earthHammerInnerScale, m_earthHammerInnerScale));

		return;
	}

	if (!m_earthHammerHasStartedImpactUI)
	{
		m_earthHammerHasStartedImpactUI = true;
		m_earthHammerImpactUITimer = 0.0f;
		m_earthHammerInnerScale = 1.0f;
	}

	m_earthHammerImpactUITimer += Time::getDeltaTime();

	const float t = std::clamp(m_earthHammerImpactUITimer / impactDuration, 0.0f, 1.0f);

	const float containerAlpha = 1.0f - MathAPI::evaluateEasing(MathAPI::EasingType::EaseInCubic, t);
	Transform2DAPI::setAlpha(m_earthHammerUIContainerTransform2D, containerAlpha);

	const float glowAlpha = 1.0f - MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);
	Transform2DAPI::setAlpha(m_earthHammerUIGlowTransform2D, glowAlpha);
	Transform2DAPI::setAlpha(m_earthHammerUISpikesTransform2D, glowAlpha);
}

void ArthurUI::hideEarthHammerUI()
{
	if (!m_earthHammerUICanvasTransform)
	{
		return;
	}

	GameObject* owner = m_earthHammerUICanvasTransform->getOwner();

	if (!owner)
	{
		return;
	}

	GameObjectAPI::setActive(owner, false);
}

IMPLEMENT_SCRIPT(ArthurUI)