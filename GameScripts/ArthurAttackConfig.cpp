#include "pch.h"
#include "ArthurAttackConfig.h"

IMPLEMENT_SCRIPT_FIELDS(ArthurAttackConfig,
    // Heavy Swipe
    FIELD_GROUP_COLLAPSE("Heavy Swipe",
        FIELD_GROUP_LABEL("Data"),
        SERIALIZED_FLOAT(m_heavySwipeDamage, "Heavy Swipe Damage", 0.0f, 9999.0f, 1.0f),
        SERIALIZED_FLOAT(m_heavySwipeRange, "Heavy Swipe Range", 0.0f, 20.0f, 0.1f),
        SERIALIZED_FLOAT(m_heavySwipeHalfAngleDegrees, "Heavy Swipe Half Angle Degrees", 0.0f, 180.0f, 1.0f),
        SERIALIZED_FLOAT(m_heavySwipeTotalDuration, "Heavy Swipe Total Duration", 0.1f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipeHit1Time, "Heavy Swipe Hit 1 Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipeHit2Time, "Heavy Swipe Hit 2 Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipeHit3Time, "Heavy Swipe Hit 3 Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipeRecoveryDuration, "Heavy Swipe Recovery Duration", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipePhase2Hit1Time, "Heavy Swipe Phase 2 Hit 1 Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipePhase2Hit2Time, "Heavy Swipe Phase 2 Hit 2 Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipePhase2Hit3Time, "Heavy Swipe Phase 2 Hit 3 Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipePhase2Hit4Time, "Heavy Swipe Phase 2 Hit 4 Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_heavySwipePhase2RecoveryDuration, "Heavy Swipe Phase 2 Recovery Duration", 0.0f, 10.0f, 0.05f),
        FIELD_GROUP_LABEL("UI"),
	    SERIALIZED_COMPONENT_REF(m_heavySwipeUICanvas, "Heavy Swipe UI Canvas", ComponentType::TRANSFORM),
	    SERIALIZED_COMPONENT_REF(m_heavySwipeUIContainer, "Heavy Swipe UI Container", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_heavySwipeUIBackground, "Heavy Swipe UI Background", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_heavySwipeUIBorder, "Heavy Swipe UI Border", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_heavySwipeUIGlow, "Heavy Swipe UI Glow", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_heavySwipeUIRightClaw, "Heavy Swipe UI Right Claw", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_heavySwipeUILeftClaw, "Heavy Swipe UI Left Claw", ComponentType::TRANSFORM2D)
    ),
    // Side Sweep
    FIELD_GROUP_COLLAPSE("Side Sweep",
        FIELD_GROUP_LABEL("Data"),
        SERIALIZED_FLOAT(m_sideSweepDamage, "Side Sweep Damage", 0.0f, 9999.0f, 1.0f),
        SERIALIZED_FLOAT(m_sideSweepRange, "Side Sweep Range", 0.0f, 30.0f, 0.1f),
        SERIALIZED_FLOAT(m_sideSweepHalfAngleDegrees, "Side Sweep Half Angle Degrees", 0.0f, 180.0f, 1.0f),
        SERIALIZED_FLOAT(m_sideSweepHitTime, "Side Sweep Hit Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_sideSweepTotalDuration, "Side Sweep Total Duration", 0.1f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_sideSweepRecoveryDuration, "Side Sweep Recovery Duration", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_sideSweepCooldown, "Side Sweep Cooldown", 0.0f, 10.0f, 0.1f),
        SERIALIZED_FLOAT(m_sideSweepPhase2HitTime, "Side Sweep Phase 2 Hit Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_sideSweepPhase2TotalDuration, "Side Sweep Phase 2 Total Duration", 0.1f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_sideSweepPhase2RecoveryDuration, "Side Sweep Phase 2 Recovery Duration", 0.0f, 10.0f, 0.05f),
        FIELD_GROUP_LABEL("UI"),
	    SERIALIZED_COMPONENT_REF(m_sideSweepUICanvas, "Side Sweep UI Canvas", ComponentType::TRANSFORM),
	    SERIALIZED_COMPONENT_REF(m_sideSweepUIContainer, "Side Sweep UI Container", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_sideSweepUIBackground, "Side Sweep UI Background", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_sideSweepUIShadow, "Side Sweep UI Shadow", ComponentType::TRANSFORM2D)
    ),
    // Charging Slam
    FIELD_GROUP_COLLAPSE("Charging Slam",
        FIELD_GROUP_LABEL("Data"),
        SERIALIZED_FLOAT(m_chargingSlamDashDamage, "Charging Slam Dash Damage", 0.0f, 9999.0f, 1.0f),
        SERIALIZED_FLOAT(m_chargingSlamFinalAreaImpactDamage, "Charging Slam Final Area Impact Damage", 0.0f, 9999.0f, 1.0f),
        SERIALIZED_FLOAT(m_chargingSlamHitTime, "Charging Slam Hit Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_chargingSlamDashSpeed, "Charging Slam Dash Speed", 0.0f, 50.0f, 0.1f),
        SERIALIZED_FLOAT(m_chargingSlamDashHitRadius, "Charging Slam Dash Hit Radius", 0.0f, 10.0f, 0.1f),
        SERIALIZED_FLOAT(m_chargingSlamImpactRadius, "Charging Slam Impact Radius", 0.0f, 20.0f, 0.1f),
        SERIALIZED_FLOAT(m_chargingSlamTotalDuration, "Charging Slam Total Duration", 0.1f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_chargingSlamRecoveryDuration, "Charging Slam Recovery Duration", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_chargingSlamImpactStunDuration, "Charging Slam Impact Stun Duration", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_chargingSlamCooldown, "Charging Slam Cooldown", 0.0f, 10.0f, 0.1f),
        SERIALIZED_FLOAT(m_chargingSlamPhase2HitTime, "Charging Slam Phase 2 Hit Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_chargingSlamPhase2DashSpeed, "Charging Slam Phase 2 Dash Speed", 0.0f, 50.0f, 0.1f),
        SERIALIZED_FLOAT(m_chargingSlamMinRange, "Charging Slam Min Range", 0.0f, 20.0f, 0.1f),
        SERIALIZED_FLOAT(m_chargingSlamMaxRange, "Charging Slam Max Range", 0.0f, 20.0f, 0.1f),
        FIELD_GROUP_LABEL("UI"),
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
    // Earth Hammer
    FIELD_GROUP_COLLAPSE("Earth Hammer",
        FIELD_GROUP_LABEL("Data"),
        SERIALIZED_FLOAT(m_earthHammerDamage, "Earth Hammer Damage", 0.0f, 9999.0f, 1.0f),
        SERIALIZED_FLOAT(m_earthHammerRadius, "Earth Hammer Radius", 0.0f, 30.0f, 0.1f),
        SERIALIZED_FLOAT(m_earthHammerTotalDuration, "Earth Hammer Total Duration", 0.1f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_earthHammerHitTime, "Earth Hammer Hit Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_earthHammerRecoveryDuration, "Earth Hammer Recovery Duration", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_earthHammerStunDuration, "Earth Hammer Stun Duration", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_earthHammerCooldown, "Earth Hammer Cooldown", 0.0f, 10.0f, 0.1f),
        SERIALIZED_FLOAT(m_earthHammerPhase2Damage, "Earth Hammer Phase 2 Damage", 0.0f, 9999.0f, 1.0f),
        SERIALIZED_FLOAT(m_earthHammerPhase2StunDuration, "Earth Hammer Phase 2 Stun Duration", 0.0f, 10.0f, 0.05f),
        FIELD_GROUP_LABEL("UI"),
	    SERIALIZED_COMPONENT_REF(m_earthHammerUICanvas, "Earth Hammer UI Canvas", ComponentType::TRANSFORM),
	    SERIALIZED_COMPONENT_REF(m_earthHammerUIContainer, "Earth Hammer UI Container", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_earthHammerUIInner, "Earth Hammer UI Inner", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_earthHammerUISpikes, "Earth Hammer UI Spikes", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_earthHammerUIGlow, "Earth Hammer UI Glow", ComponentType::TRANSFORM2D),
	    SERIALIZED_COMPONENT_REF(m_earthHammerUIRing, "Earth Hammer UI Ring", ComponentType::TRANSFORM2D)
    )
)

ArthurAttackConfig::ArthurAttackConfig(GameObject* owner)
    : Script(owner)
{
}

void ArthurAttackConfig::Start()
{
	// Heavy Swipe UI
	m_heavySwipeUICanvasTransform = m_heavySwipeUICanvas.getReferencedComponent();
	m_heavySwipeUIContainerTransform2D = m_heavySwipeUIContainer.getReferencedComponent();
	m_heavySwipeUIBackgroundTransform2D = m_heavySwipeUIBackground.getReferencedComponent();
	m_heavySwipeUIBorderTransform2D = m_heavySwipeUIBorder.getReferencedComponent();
	m_heavySwipeUIGlowTransform2D = m_heavySwipeUIGlow.getReferencedComponent();
	m_heavySwipeUIRightClawTransform2D = m_heavySwipeUIRightClaw.getReferencedComponent();
    m_heavySwipeUILeftClawTransform2D = m_heavySwipeUILeftClaw.getReferencedComponent();
    if (m_heavySwipeUICanvasTransform)
    {
        GameObjectAPI::setActive(m_heavySwipeUICanvasTransform->getOwner(), false);
    }

	// Side Sweep UI
	m_sideSweepUICanvasTransform = m_sideSweepUICanvas.getReferencedComponent();
	m_sideSweepUIContainerTransform2D = m_sideSweepUIContainer.getReferencedComponent();
	m_sideSweepUIBackgroundTransform2D = m_sideSweepUIBackground.getReferencedComponent();
	m_sideSweepUIShadowTransform2D = m_sideSweepUIShadow.getReferencedComponent();
    if (m_sideSweepUICanvasTransform)
    {
        GameObjectAPI::setActive(m_sideSweepUICanvasTransform->getOwner(), false);
    }

	// Charging Slam UI
	m_chargingSlamUICanvasTransform = m_chargingSlamUICanvas.getReferencedComponent();
    m_chargingSlamUIContainerTransform2D = m_chargingSlamUIContainer.getReferencedComponent();
	m_chargingSlamUIBackgroundTransform2D = m_chargingSlamUIBackground.getReferencedComponent();
	m_chargingSlamUIBordersTransform2D = m_chargingSlamUIBorders.getReferencedComponent();
	m_chargingSlamUIShadowTransform2D = m_chargingSlamUIShadow.getReferencedComponent();
	m_chargingSlamUISpikesTransform2D = m_chargingSlamUISpikes.getReferencedComponent();
	m_chargingSlamUIBordersSliderComponent = m_chargingSlamUIBordersSlider.getReferencedComponent();
	m_chargingSlamUIShadowSliderComponent = m_chargingSlamUIShadowSlider.getReferencedComponent();
    if (m_chargingSlamUICanvasTransform)
    {
        GameObjectAPI::setActive(m_chargingSlamUICanvasTransform->getOwner(), false);
    }

    m_chargingSlamImpactUICanvasTransform = m_chargingSlamImpactUICanvas.getReferencedComponent();
    m_chargingSlamImpactUIContainerTransform2D = m_chargingSlamImpactUIContainer.getReferencedComponent();
    m_chargingSlamImpactUICenterTransform2D = m_chargingSlamImpactUICenter.getReferencedComponent();
    m_chargingSlamImpactUIGlowTransform2D = m_chargingSlamImpactUIGlow.getReferencedComponent();
    if (m_chargingSlamImpactUICanvasTransform)
    {
        GameObjectAPI::setActive(m_chargingSlamImpactUICanvasTransform->getOwner(), false);
    }

	// Earth Hammer UI
    m_earthHammerUICanvasTransform = m_earthHammerUICanvas.getReferencedComponent();
	m_earthHammerUIContainerTransform2D = m_earthHammerUIContainer.getReferencedComponent();
    m_earthHammerUIInnerTransform2D = m_earthHammerUIInner.getReferencedComponent();
    m_earthHammerUISpikesTransform2D = m_earthHammerUISpikes.getReferencedComponent();
    m_earthHammerUIGlowTransform2D = m_earthHammerUIGlow.getReferencedComponent();
    m_earthHammerUIRingTransform2D = m_earthHammerUIRing.getReferencedComponent();
    if (m_earthHammerUICanvasTransform)
    {
        GameObjectAPI::setActive(m_earthHammerUICanvasTransform->getOwner(), false);
    }
}

IMPLEMENT_SCRIPT(ArthurAttackConfig)