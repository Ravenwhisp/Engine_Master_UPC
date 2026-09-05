#include "pch.h"
#include "AelorinUI.h"

#include "Transform2D.h"

#include <algorithm>
#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(AelorinUI,
	FIELD_GROUP_COLLAPSE("Seeker Sigils",
		SERIALIZED_COMPONENT_REF(m_seekerSigilsUICanvas, "Seeker Sigils UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_seekerSigilsUIContainer, "Seeker Sigils UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_seekerSigilsUIBackground, "Seeker Sigils UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_seekerSigilsUIBorder, "Seeker Sigils UI Border", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_seekerSigilsUIGlow, "Seeker Sigils UI Glow", ComponentType::TRANSFORM2D)
	),

	FIELD_GROUP_COLLAPSE("Nova",
		SERIALIZED_COMPONENT_REF(m_novaUICanvas, "Nova UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_novaUIContainer, "Nova UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_novaUIBackground, "Nova UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_novaUIBorder, "Nova UI Border", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_novaUIGlow, "Nova UI Glow", ComponentType::TRANSFORM2D)
	),

	FIELD_GROUP_COLLAPSE("Risen Spires",
		SERIALIZED_COMPONENT_REF(m_risenSpiresUICanvas, "Risen Spires UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_risenSpiresUIContainer, "Risen Spires UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_risenSpiresUIBackground, "Risen Spires UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_risenSpiresUIBorder, "Risen Spires UI Border", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_risenSpiresUIGlow, "Risen Spires UI Glow", ComponentType::TRANSFORM2D)
	),

	FIELD_GROUP_COLLAPSE("Spirit Cannon",
		SERIALIZED_COMPONENT_REF(m_spiritCannonUICanvas, "Spirit Cannon UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_spiritCannonUIContainer, "Spirit Cannon UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_spiritCannonUIBackground, "Spirit Cannon UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_spiritCannonUIBorder, "Spirit Cannon UI Border", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_spiritCannonUIGlow, "Spirit Cannon UI Glow", ComponentType::TRANSFORM2D)
	),

	FIELD_GROUP_COLLAPSE("Grasp of the Dead",
		SERIALIZED_COMPONENT_REF(m_graspOfTheDeadUICanvas, "Grasp of the Dead UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_graspOfTheDeadUIContainer, "Grasp of the Dead UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_graspOfTheDeadUIBackground, "Grasp of the Dead UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_graspOfTheDeadUIBorder, "Grasp of the Dead UI Border", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_graspOfTheDeadUIGlow, "Grasp of the Dead UI Glow", ComponentType::TRANSFORM2D)
	),

	FIELD_GROUP_COLLAPSE("Soul Cataclysm",
		SERIALIZED_COMPONENT_REF(m_soulCataclysmUICanvas, "Soul Cataclysm UI Canvas",	ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_soulCataclysmUIContainer, "Soul Cataclysm UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_soulCataclysmUIBackground, "Soul Cataclysm UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_soulCataclysmUIBorder, "Soul Cataclysm UI Border",	ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_soulCataclysmUIGlow, "Soul Cataclysm UI Glow", ComponentType::TRANSFORM2D),

		SERIALIZED_COMPONENT_REF(m_soulCataclysmSafeZoneUICanvas, "Soul Cataclysm Safe Zone UI Canvas", ComponentType::TRANSFORM),
		SERIALIZED_COMPONENT_REF(m_soulCataclysmSafeZoneUIContainer, "Soul Cataclysm Safe Zone UI Container", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_soulCataclysmSafeZoneUIBackground, "Soul Cataclysm Safe Zone UI Background", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_soulCataclysmSafeZoneUIBorder, "Soul Cataclysm Safe Zone UI Border", ComponentType::TRANSFORM2D),
		SERIALIZED_COMPONENT_REF(m_soulCataclysmSafeZoneUIGlow, "Soul Cataclysm Safe Zone UI Glow",	ComponentType::TRANSFORM2D)
	)
)

AelorinUI::AelorinUI(GameObject* owner) 
	: Script(owner)
{
}

void AelorinUI::Start()
{
	// Seeker Sigils
	m_seekerSigilsUICanvasTransform = m_seekerSigilsUICanvas.getReferencedComponent();
	m_seekerSigilsUIContainerTransform2D = m_seekerSigilsUIContainer.getReferencedComponent();
	m_seekerSigilsUIBackgroundTransform2D = m_seekerSigilsUIBackground.getReferencedComponent();
	m_seekerSigilsUIBorderTransform2D = m_seekerSigilsUIBorder.getReferencedComponent();
	m_seekerSigilsUIGlowTransform2D = m_seekerSigilsUIGlow.getReferencedComponent();

	setupSeekerSigilsUI();
	hideAllSeekerSigilsUI();

	// Nova
	m_novaUICanvasTransform = m_novaUICanvas.getReferencedComponent();
	m_novaUIContainerTransform2D = m_novaUIContainer.getReferencedComponent();
	m_novaUIBackgroundTransform2D = m_novaUIBackground.getReferencedComponent();
	m_novaUIBorderTransform2D = m_novaUIBorder.getReferencedComponent();
	m_novaUIGlowTransform2D = m_novaUIGlow.getReferencedComponent();

	hideNovaUI();

	// Risen Spires
	m_risenSpiresUICanvasTransform = m_risenSpiresUICanvas.getReferencedComponent();
	m_risenSpiresUIContainerTransform2D = m_risenSpiresUIContainer.getReferencedComponent();
	m_risenSpiresUIBackgroundTransform2D = m_risenSpiresUIBackground.getReferencedComponent();
	m_risenSpiresUIBorderTransform2D = m_risenSpiresUIBorder.getReferencedComponent();
	m_risenSpiresUIGlowTransform2D = m_risenSpiresUIGlow.getReferencedComponent();

	setupRisenSpiresUI();
	hideAllRisenSpiresUI();

	// Spirit Cannon
	m_spiritCannonUICanvasTransform = m_spiritCannonUICanvas.getReferencedComponent();
	m_spiritCannonUIContainerTransform2D = m_spiritCannonUIContainer.getReferencedComponent();
	m_spiritCannonUIBackgroundTransform2D = m_spiritCannonUIBackground.getReferencedComponent();
	m_spiritCannonUIBorderTransform2D = m_spiritCannonUIBorder.getReferencedComponent();
	m_spiritCannonUIGlowTransform2D = m_spiritCannonUIGlow.getReferencedComponent();

	hideSpiritCannonUI();

	// Grasp of the Dead
	m_graspOfTheDeadUICanvasTransform =	m_graspOfTheDeadUICanvas.getReferencedComponent();
	m_graspOfTheDeadUIContainerTransform2D = m_graspOfTheDeadUIContainer.getReferencedComponent();
	m_graspOfTheDeadUIBackgroundTransform2D = m_graspOfTheDeadUIBackground.getReferencedComponent();
	m_graspOfTheDeadUIBorderTransform2D = m_graspOfTheDeadUIBorder.getReferencedComponent();
	m_graspOfTheDeadUIGlowTransform2D =	m_graspOfTheDeadUIGlow.getReferencedComponent();

	hideGraspOfTheDeadUI();

	// Soul Cataclysm - Arena
	m_soulCataclysmUICanvasTransform = m_soulCataclysmUICanvas.getReferencedComponent();
	m_soulCataclysmUIContainerTransform2D =	m_soulCataclysmUIContainer.getReferencedComponent();
	m_soulCataclysmUIBackgroundTransform2D = m_soulCataclysmUIBackground.getReferencedComponent();
	m_soulCataclysmUIBorderTransform2D = m_soulCataclysmUIBorder.getReferencedComponent();
	m_soulCataclysmUIGlowTransform2D = m_soulCataclysmUIGlow.getReferencedComponent();

	// Soul Cataclysm - Safe Zone
	m_soulCataclysmSafeZoneUICanvasTransform = m_soulCataclysmSafeZoneUICanvas.getReferencedComponent();
	m_soulCataclysmSafeZoneUIContainerTransform2D = m_soulCataclysmSafeZoneUIContainer.getReferencedComponent();
	m_soulCataclysmSafeZoneUIBackgroundTransform2D = m_soulCataclysmSafeZoneUIBackground.getReferencedComponent();
	m_soulCataclysmSafeZoneUIBorderTransform2D = m_soulCataclysmSafeZoneUIBorder.getReferencedComponent();
	m_soulCataclysmSafeZoneUIGlowTransform2D = m_soulCataclysmSafeZoneUIGlow.getReferencedComponent();

	setupSoulCataclysmSafeZonesUI();
	hideSoulCataclysmUI();
}

void AelorinUI::Update()
{
	const float deltaTime = Time::getDeltaTime();

	updateSeekerSigilsUI(deltaTime);
	updateNovaUI(deltaTime);
	updateRisenSpiresUI(deltaTime);
	updateSpiritCannonUI(deltaTime);
	updateGraspOfTheDeadUI(deltaTime);
	updateSoulCataclysmUI(deltaTime);
}

void AelorinUI::showSeekerSigilsUI(const Vector3& impactPosition, float radius, float telegraphDuration)
{
	SeekerSigilsUISlot* slot = acquireSeekerSigilsUISlot();

	if (!slot ||
		!slot->canvas ||
		!slot->container ||
		!slot->background ||
		!slot->border ||
		!slot->glow)
	{
		return;
	}

	GameObject* canvasObject = ComponentAPI::getOwner(slot->canvas);
	if (!canvasObject)
	{
		return;
	}

	slot->active = true;
	slot->timer = 0.0f;
	slot->duration = (std::max)(telegraphDuration, 0.001f);

	GameObjectAPI::setActive(canvasObject, true);

	Vector3 uiPosition = impactPosition;
	uiPosition.y += 0.05f;

	TransformAPI::setGlobalPosition(slot->canvas, uiPosition);
	TransformAPI::setGlobalRotationEuler(slot->canvas, Vector3(90.0f, 0.0f, 0.0f));

	Transform2DAPI::setScale(slot->container, Vector2(radius, radius));	
	Transform2DAPI::setAlpha(slot->container, 0.0f);
	Transform2DAPI::setAlpha(slot->background, 1.0f);
	Transform2DAPI::setAlpha(slot->border, 1.0f);
	Transform2DAPI::setAlpha(slot->glow, 0.0f);

	Transform2DAPI::setScale(slot->background, Vector2(0.1f, 0.1f));
}

void AelorinUI::setNovaContainerRadius(float radius)
{
	if (!m_novaUIContainerTransform2D)
	{
		return;
	}

	const float baseDiameterUI = Transform2DAPI::getBaseSize(m_novaUIContainerTransform2D).x;
	if (baseDiameterUI <= 0.001f)
	{
		return;
	}

	const float desiredDiameterUI = radius * 2.0f * 100.0f;
	const float scale = desiredDiameterUI / baseDiameterUI;

	Transform2DAPI::setScale(m_novaUIContainerTransform2D, Vector2(scale, scale));
}

void AelorinUI::showNovaUI(const Vector3& center, float firstRadius, float firstChargeDuration, bool hasSecondWave, float secondRadius, float secondChargeDuration)
{
	if (!m_novaUICanvasTransform ||
		!m_novaUIContainerTransform2D ||
		!m_novaUIBackgroundTransform2D ||
		!m_novaUIBorderTransform2D ||
		!m_novaUIGlowTransform2D)
	{
		return;
	}

	GameObject* canvasObject = ComponentAPI::getOwner(m_novaUICanvasTransform);
	if (!canvasObject)
	{
		return;
	}

	m_novaUIActive = true;
	m_novaUIHasSecondWave = hasSecondWave && secondRadius > firstRadius && secondChargeDuration > 0.0f;
	m_novaUISecondWaveStarted = false;
	m_novaUITimer = 0.0f;
	m_novaUIFirstRadius = firstRadius;
	m_novaUISecondRadius = secondRadius;
	m_novaUIFirstChargeDuration = (std::max)(firstChargeDuration, 0.001f);
	m_novaUISecondChargeDuration = (std::max)(secondChargeDuration, 0.001f);

	GameObjectAPI::setActive(canvasObject, true);

	Vector3 uiPosition = center;
	uiPosition.y += 0.05f;

	TransformAPI::setGlobalPosition(m_novaUICanvasTransform, uiPosition);
	TransformAPI::setGlobalRotationEuler(m_novaUICanvasTransform, Vector3(90.0f, 0.0f, 0.0f));

	// start with wave 1 gameplay radius
	setNovaContainerRadius(firstRadius);

	// reset visuals
	Transform2DAPI::setAlpha(m_novaUIContainerTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_novaUIBorderTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_novaUIBackgroundTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_novaUIGlowTransform2D, 0.0f);

	// wave 1 starts from the center and expands until it reaches the first damage radius
	Transform2DAPI::setScale(m_novaUIBackgroundTransform2D, Vector2(0.1f, 0.1f));

	// glow uses the full current radius
	// in phase 2 resize it at wave 1 impact
	Transform2DAPI::setScale(m_novaUIGlowTransform2D, Vector2(1.0f, 1.0f));
}

void AelorinUI::showRisenSpiresUI(Transform* patternRoot, float radius, float chargeDuration)
{
	if (!patternRoot)
	{
		return;
	}

	hideAllRisenSpiresUI();

	m_risenSpiresUIActive = true;
	m_risenSpiresUITimer = 0.0f;
	m_risenSpiresUIChargeDuration = (std::max)(chargeDuration, 0.001f);

	const int pointCount = TransformAPI::getChildCount(patternRoot);

	for (int i = 0; i < pointCount; ++i)
	{
		Transform* spirePoint = TransformAPI::getChild(patternRoot, i);
		if (!spirePoint)
		{
			continue;
		}

		RisenSpiresUISlot* slot = acquireRisenSpiresUISlot();
		if (!slot)
		{
			Debug::warn("[AelorinUI] Not enough Risen Spires UI slots for pattern");
			break;
		}

		if (!slot->canvas ||
			!slot->container ||
			!slot->background ||
			!slot->border ||
			!slot->glow)
		{
			continue;
		}

		GameObject* slotObject = ComponentAPI::getOwner(slot->canvas);
		if (!slotObject)
		{
			continue;
		}

		Vector3 uiPosition = TransformAPI::getGlobalPosition(spirePoint);
		uiPosition.y += 0.05f;
		
		GameObjectAPI::setActive(slotObject, true);
		TransformAPI::setGlobalPosition(slot->canvas, uiPosition);
		TransformAPI::setGlobalRotationEuler(slot->canvas, Vector3(90.0f, 0.0f, 0.0f));

		setRisenSpiresSlotRadius(*slot, radius);

		// reset visuals
		Transform2DAPI::setAlpha(slot->container, 1.0f);
		Transform2DAPI::setAlpha(slot->background, 1.0f);
		Transform2DAPI::setAlpha(slot->border, 1.0f);
		Transform2DAPI::setAlpha(slot->glow, 0.0f);

		// telegraph starts from the center
		Transform2DAPI::setScale(slot->background, Vector2(0.1f, 0.1f));
		Transform2DAPI::setScale(slot->glow, Vector2(1.0f, 1.0f));

		slot->active = true;
	}
}

void AelorinUI::showSpiritCannonUI(Transform* originTransform, Transform* targetTransform, float beamLength, float beamWidth, float chargeDuration)
{
	GameObject* canvasObject = ComponentAPI::getOwner(m_spiritCannonUICanvasTransform);
	if (!canvasObject)
	{
		return;
	}

	m_spiritCannonOriginTransform = originTransform;
	m_spiritCannonTargetTransform =	targetTransform;
	m_spiritCannonBeamLength = beamLength;
	m_spiritCannonBeamWidth = beamWidth;
	m_spiritCannonUITimer = 0.0f;
	m_spiritCannonUIChargeDuration = (std::max)(chargeDuration, 0.001f);
	m_spiritCannonUIActive = true;
	m_spiritCannonUICharging = true;

	GameObjectAPI::setActive(canvasObject, true);

	setSpiritCannonSize(beamLength, beamWidth);

	Transform2DAPI::setAlpha(m_spiritCannonUIContainerTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_spiritCannonUIBorderTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_spiritCannonUIBackgroundTransform2D, 0.0f);
	Transform2DAPI::setAlpha(m_spiritCannonUIGlowTransform2D, 0.0f);
	Transform2DAPI::setScale(m_spiritCannonUIBackgroundTransform2D,	Vector2(1.0f, 1.0f));
}

void AelorinUI::showGraspOfTheDeadUI(const Vector3& center, float radius, float pullDuration)
{
	if (!m_graspOfTheDeadUICanvasTransform ||
		!m_graspOfTheDeadUIContainerTransform2D ||
		!m_graspOfTheDeadUIBackgroundTransform2D ||
		!m_graspOfTheDeadUIBorderTransform2D ||
		!m_graspOfTheDeadUIGlowTransform2D)
	{
		return;
	}

	GameObject* canvasObject = ComponentAPI::getOwner(m_graspOfTheDeadUICanvasTransform);
	if (!canvasObject)
	{
		return;
	}

	m_graspOfTheDeadUIActive = true;
	m_graspOfTheDeadUITimer = 0.0f;
	m_graspOfTheDeadUIDuration = (std::max)(pullDuration, 0.001f);

	GameObjectAPI::setActive(canvasObject, true);

	Vector3 uiPosition = center;
	uiPosition.y += 0.05f;

	TransformAPI::setGlobalPosition(m_graspOfTheDeadUICanvasTransform, uiPosition);
	TransformAPI::setGlobalRotationEuler(m_graspOfTheDeadUICanvasTransform, Vector3(90.0f, 0.0f, 0.0f));

	setGraspOfTheDeadRadius(radius);

	// reset visuals
	Transform2DAPI::setAlpha(m_graspOfTheDeadUIContainerTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_graspOfTheDeadUIBackgroundTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_graspOfTheDeadUIBorderTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_graspOfTheDeadUIGlowTransform2D, 0.0f);

	// grasp starts at full radius
	Transform2DAPI::setScale(m_graspOfTheDeadUIBackgroundTransform2D, Vector2(1.0f, 1.0f));
	Transform2DAPI::setScale(m_graspOfTheDeadUIBorderTransform2D, Vector2(1.0f, 1.0f));
	Transform2DAPI::setScale(m_graspOfTheDeadUIGlowTransform2D, Vector2(1.0f, 1.0f));

}

void AelorinUI::showSoulCataclysmUI(const Vector3& center, float radius, Transform* safeZonesRoot, float safeZoneRadius, float channelDuration)
{
	if (!m_soulCataclysmUICanvasTransform ||
		!m_soulCataclysmUIContainerTransform2D ||
		!m_soulCataclysmUIBackgroundTransform2D ||
		!m_soulCataclysmUIBorderTransform2D ||
		!m_soulCataclysmUIGlowTransform2D ||
		!safeZonesRoot)
	{
		return;
	}

	GameObject* arenaObject = ComponentAPI::getOwner(m_soulCataclysmUICanvasTransform);
	if (!arenaObject)
	{
		return;
	}

	// reset previous visualization
	hideAllSoulCataclysmSafeZonesUI();

	m_soulCataclysmUIActive = true;
	m_soulCataclysmUITimer = 0.0f;
	m_soulCataclysmUIChannelDuration = (std::max)(channelDuration, 0.001f);

	// arena
	GameObjectAPI::setActive(arenaObject, true);

	Vector3 arenaPosition = center;
	arenaPosition.y += 0.05f;

	TransformAPI::setGlobalPosition(m_soulCataclysmUICanvasTransform, arenaPosition);
	TransformAPI::setGlobalRotationEuler(m_soulCataclysmUICanvasTransform, Vector3(90.0f, 0.0f, 0.0f));

	setSoulCataclysmArenaRadius(radius);

	Transform2DAPI::setAlpha(m_soulCataclysmUIContainerTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_soulCataclysmUIBackgroundTransform2D, 0.25f);
	Transform2DAPI::setAlpha(m_soulCataclysmUIBorderTransform2D, 1.0f);
	Transform2DAPI::setAlpha(m_soulCataclysmUIGlowTransform2D, 0.0f);

	// safe zones
	const int safeZoneCount = TransformAPI::getChildCount(safeZonesRoot);

	for (int i = 0; i < safeZoneCount; ++i)
	{
		Transform* safeZoneAnchor = TransformAPI::getChild(safeZonesRoot, i);
		if (!safeZoneAnchor)
		{
			continue;
		}

		SoulCataclysmSafeZoneUISlot* slot =	acquireSoulCataclysmSafeZoneUISlot();
		if (!slot)
		{
			Debug::warn("[AelorinUI] Not enough Soul Cataclysm Safe Zone UI slots");

			break;
		}

		GameObject* slotObject = ComponentAPI::getOwner(slot->canvas);
		if (!slotObject)
		{
			continue;
		}

		Vector3 safePosition = TransformAPI::getGlobalPosition(safeZoneAnchor);
		safePosition.y += 0.06f;

		GameObjectAPI::setActive(slotObject, true);

		TransformAPI::setGlobalPosition(slot->canvas, safePosition);
		TransformAPI::setGlobalRotationEuler(slot->canvas, Vector3(90.0f, 0.0f, 0.0f));

		setSoulCataclysmSafeZoneRadius(*slot, safeZoneRadius);

		Transform2DAPI::setAlpha(slot->container, 1.0f);		
		Transform2DAPI::setAlpha(slot->background, 1.0f);
		Transform2DAPI::setAlpha(slot->border, 1.0f);
		Transform2DAPI::setAlpha(slot->glow, 0.35f);

		slot->active = true;
	}
}

void AelorinUI::setupSeekerSigilsUI()
{
	m_seekerSigilsUISlots.clear();

	// first UI is assigned in the editor and acts as a template for the remaining slots
	if (!m_seekerSigilsUICanvasTransform ||
		!m_seekerSigilsUIContainerTransform2D ||
		!m_seekerSigilsUIBackgroundTransform2D ||
		!m_seekerSigilsUIBorderTransform2D ||
		!m_seekerSigilsUIGlowTransform2D)
	{
		Debug::warn("[AelorinUI] Seeker Sigils template UI is incomplete");
		return;
	}

	// add the first slot
	SeekerSigilsUISlot firstSlot;
	firstSlot.canvas = m_seekerSigilsUICanvasTransform;
	firstSlot.container = m_seekerSigilsUIContainerTransform2D;
	firstSlot.background = m_seekerSigilsUIBackgroundTransform2D;
	firstSlot.border = m_seekerSigilsUIBorderTransform2D;
	firstSlot.glow = m_seekerSigilsUIGlowTransform2D;

	m_seekerSigilsUISlots.push_back(firstSlot);

	// the rest are siblings to the first canvas
	Transform* uiRoot = TransformAPI::getParent(m_seekerSigilsUICanvasTransform);
	if (!uiRoot)
	{
		Debug::warn("[AelorinUI] Seeker Sigils UI root not found");
		return;
	}

	const int childCount = TransformAPI::getChildCount(uiRoot);

	for (int i = 0; i < childCount; ++i)
	{
		Transform* canvas = TransformAPI::getChild(uiRoot, i);
		if (!canvas)
		{
			continue;
		}

		// first one was already added
		if (canvas == m_seekerSigilsUICanvasTransform)
		{
			continue;
		}

		Transform* containerTransform = TransformAPI::findChildByName(canvas, "Container");
		if (!containerTransform)
		{
			continue;
		}

		Transform* backgroundTransform = TransformAPI::findChildByName(containerTransform, "Background");
		Transform* borderTransform = TransformAPI::findChildByName(containerTransform, "Border");
		Transform* glowTransform = TransformAPI::findChildByName(containerTransform, "Glow");

		if (!backgroundTransform || !borderTransform || !glowTransform)
		{
			continue;
		}

		GameObject* containerObject = ComponentAPI::getOwner(containerTransform);
		GameObject* backgroundObject = ComponentAPI::getOwner(backgroundTransform);
		GameObject* borderObject = ComponentAPI::getOwner(borderTransform);
		GameObject* glowObject = ComponentAPI::getOwner(glowTransform);

		if (!containerObject || !backgroundObject || !borderObject || !glowObject)
		{
			continue;
		}

		Transform2D* container = static_cast<Transform2D*>(GameObjectAPI::getComponent(containerObject, ComponentType::TRANSFORM2D));
		Transform2D* background = static_cast<Transform2D*>(GameObjectAPI::getComponent(backgroundObject, ComponentType::TRANSFORM2D));
		Transform2D* border = static_cast<Transform2D*>(GameObjectAPI::getComponent(borderObject, ComponentType::TRANSFORM2D));
		Transform2D* glow =	static_cast<Transform2D*>(GameObjectAPI::getComponent(glowObject, ComponentType::TRANSFORM2D));

		if (!container || !background || !border || !glow)
		{
			continue;
		}

		SeekerSigilsUISlot slot;
		slot.canvas = canvas;
		slot.container = container;
		slot.background = background;
		slot.border = border;
		slot.glow = glow;

		m_seekerSigilsUISlots.push_back(slot);
	}

	Debug::log("[AelorinUI] Cached %d Seeker Sigils UI slots", static_cast<int>(m_seekerSigilsUISlots.size()));
}

void AelorinUI::hideAllSeekerSigilsUI()
{
	for (SeekerSigilsUISlot& slot : m_seekerSigilsUISlots)
	{
		hideSeekerSigilsUISlot(slot);
	}
}

void AelorinUI::updateSeekerSigilsUI(float deltaTime)
{
	for (SeekerSigilsUISlot& slot : m_seekerSigilsUISlots)
	{
		if (!slot.active)
		{
			continue;
		}

		if (!slot.container ||
			!slot.background ||
			!slot.border ||
			!slot.glow)
		{
			hideSeekerSigilsUISlot(slot);
			continue;
		}

		slot.timer += deltaTime;

		if (slot.timer < slot.duration)
		{
			const float t = std::clamp(slot.timer / slot.duration, 0.0f, 1.0f);
			const float containerAlpha = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);
			Transform2DAPI::setAlpha(slot.container, containerAlpha);

			const float backgroundScale = 0.1f + (t * 0.9f);
			Transform2DAPI::setScale(slot.background, Vector2(backgroundScale, backgroundScale));

			continue;
		}

		const float impactTimer = slot.timer - slot.duration;
		const float impactT = std::clamp(impactTimer / m_seekerSigilsImpactFadeDuration, 0.0f, 1.0f);
		Transform2DAPI::setScale(slot.background, Vector2(1.0f, 1.0f));

		const float fadeAlpha = 1.0f - MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, impactT);
		Transform2DAPI::setAlpha(slot.container, fadeAlpha);

		Transform2DAPI::setAlpha(slot.glow, fadeAlpha);

		if (impactT >= 1.0f)
		{
			hideSeekerSigilsUISlot(slot);
		}
	}
}

void AelorinUI::hideSeekerSigilsUISlot(SeekerSigilsUISlot& slot)
{
	if (slot.canvas)
	{
		GameObject* canvasObject = ComponentAPI::getOwner(slot.canvas);
		if (canvasObject)
		{
			GameObjectAPI::setActive(canvasObject, false);
		}
	}

	slot.active = false;
	slot.timer = 0.0f;
	slot.duration = 0.0f;
}

AelorinUI::SeekerSigilsUISlot* AelorinUI::acquireSeekerSigilsUISlot()
{
	for (SeekerSigilsUISlot& slot : m_seekerSigilsUISlots)
	{
		if (!slot.active)
		{
			return &slot;
		}
	}

	return nullptr;
}

void AelorinUI::hideNovaUI()
{
	if (m_novaUICanvasTransform)
	{
		GameObject* canvasObject = ComponentAPI::getOwner(m_novaUICanvasTransform);
		if (canvasObject)
		{
			GameObjectAPI::setActive(canvasObject, false);
		}
	}

	m_novaUIActive = false;
	m_novaUIHasSecondWave = false;
	m_novaUISecondWaveStarted = false;
	m_novaUITimer = 0.0f;
	m_novaUIFirstChargeDuration = 0.0f;
	m_novaUISecondChargeDuration = 0.0f;
	m_novaUIFirstRadius = 0.0f;
	m_novaUISecondRadius = 0.0f;
}

void AelorinUI::updateNovaUI(float deltaTime)
{
	if (!m_novaUIActive)
	{
		return;
	}

	if (!m_novaUIContainerTransform2D ||
		!m_novaUIBackgroundTransform2D ||
		!m_novaUIGlowTransform2D)
	{
		hideNovaUI();
		return;
	}

	m_novaUITimer += deltaTime;

	// wave 1 charge
	if (!m_novaUISecondWaveStarted && m_novaUITimer < m_novaUIFirstChargeDuration)
	{
		const float t = std::clamp(m_novaUITimer / m_novaUIFirstChargeDuration, 0.0f, 1.0f);
		const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);
		const float fillScale = 0.1f + 0.9f * easedT;

		Transform2DAPI::setScale(m_novaUIBackgroundTransform2D, Vector2(fillScale, fillScale));

		return;
	}

	// phase 1 - wave 1 is the final impact
	if (!m_novaUIHasSecondWave)
	{
		const float impactTimer = m_novaUITimer - m_novaUIFirstChargeDuration;
		const float impactT = std::clamp(impactTimer / m_novaUIImpactFadeDuration, 0.0f, 1.0f);

		Transform2DAPI::setScale(m_novaUIBackgroundTransform2D, Vector2(1.0f, 1.0f));
		Transform2DAPI::setAlpha(m_novaUIGlowTransform2D, 1.0f - impactT);
		Transform2DAPI::setAlpha(m_novaUIContainerTransform2D, 1.0f - impactT);

		if (impactT >= 1.0f)
		{
			hideNovaUI();
		}

		return;
	}
	
	// phase 2 - wave 1 impact -> continue outward immediately
	if (!m_novaUISecondWaveStarted)
	{
		m_novaUISecondWaveStarted = true;

		// background visually at wave 1 radius after container becomes wave 2 size
		const float firstToSecondRatio = m_novaUISecondRadius > 0.001f ? m_novaUIFirstRadius / m_novaUISecondRadius : 1.0f;

		// border instantly becomes wave 2 radius
		setNovaContainerRadius(m_novaUISecondRadius);

		// continue charge from wave 1 radius
		Transform2DAPI::setScale(m_novaUIBackgroundTransform2D, Vector2(firstToSecondRatio, firstToSecondRatio));
		Transform2DAPI::setScale(m_novaUIGlowTransform2D, Vector2(firstToSecondRatio, firstToSecondRatio));
		Transform2DAPI::setAlpha(m_novaUIGlowTransform2D, 1.0f);

		return;
	}

	// wave 2 charge
	const float secondWaveTimer = m_novaUITimer - m_novaUIFirstChargeDuration;
	if (secondWaveTimer < m_novaUISecondChargeDuration)
	{
		const float t = std::clamp(secondWaveTimer / m_novaUISecondChargeDuration, 0.0f, 1.0f);
		const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);
		const float startScale = m_novaUISecondRadius > 0.001f ? m_novaUIFirstRadius / m_novaUISecondRadius : 1.0f;
		const float fillScale = startScale + (1.0f - startScale) * easedT;

		Transform2DAPI::setScale(m_novaUIBackgroundTransform2D, Vector2(fillScale, fillScale));

		// fade wave 1 flash quickly
		const float glowT = std::clamp(secondWaveTimer / m_seekerSigilsImpactFadeDuration, 0.0f, 1.0f);
		
		Transform2DAPI::setAlpha(m_novaUIGlowTransform2D, 1.0f - glowT);

		return;
	}

	// wave 2 impact
	Transform2DAPI::setScale(m_novaUIBackgroundTransform2D, Vector2(1.0f, 1.0f));
	Transform2DAPI::setScale(m_novaUIGlowTransform2D, Vector2(1.0f, 1.0f));

	const float finalImpactTimer = secondWaveTimer - m_novaUISecondChargeDuration;
	const float finalImpactT = std::clamp(finalImpactTimer / m_novaUIImpactFadeDuration, 0.0f, 1.0f);

	Transform2DAPI::setAlpha(m_novaUIGlowTransform2D, 1.0f - finalImpactT);
	Transform2DAPI::setAlpha(m_novaUIContainerTransform2D, 1.0f - finalImpactT);

	if (finalImpactT >= 1.0f)
	{
		hideNovaUI();
	}
}

void AelorinUI::hideAllRisenSpiresUI()
{
	for (RisenSpiresUISlot& slot : m_risenSpiresUISlots)
	{
		hideRisenSpiresUISlot(slot);
	}

	m_risenSpiresUIActive = false;
	m_risenSpiresUITimer = 0.0f;
	m_risenSpiresUIChargeDuration = 0.0f;
}

void AelorinUI::setupRisenSpiresUI()
{
	m_risenSpiresUISlots.clear();

	// first UI is assigned in the editor and acts as a template for the remaining slots
	if (!m_risenSpiresUICanvasTransform ||
		!m_risenSpiresUIContainerTransform2D ||
		!m_risenSpiresUIBackgroundTransform2D ||
		!m_risenSpiresUIBorderTransform2D ||
		!m_risenSpiresUIGlowTransform2D)
	{
		Debug::warn("[AelorinUI] Risen Spires template UI is incomplete");
		return;
	}

	// add the first slot
	RisenSpiresUISlot firstSlot;
	firstSlot.canvas = m_risenSpiresUICanvasTransform;
	firstSlot.container = m_risenSpiresUIContainerTransform2D;
	firstSlot.background = m_risenSpiresUIBackgroundTransform2D;
	firstSlot.border = m_risenSpiresUIBorderTransform2D;
	firstSlot.glow = m_risenSpiresUIGlowTransform2D;
	firstSlot.active = false;

	m_risenSpiresUISlots.push_back(firstSlot);

	// the rest are siblings to the first canvas
	Transform* uiRoot = TransformAPI::getParent(m_risenSpiresUICanvasTransform);
	if (!uiRoot)
	{
		Debug::warn("[AelorinUI] Risen Spires UI root not found");
		return;
	}

	const int childCount = TransformAPI::getChildCount(uiRoot);

	for (int i = 0; i < childCount; ++i)
	{
		Transform* canvas = TransformAPI::getChild(uiRoot, i);
		if (!canvas)
		{
			continue;
		}

		// first one was already added
		if (canvas == m_risenSpiresUICanvasTransform)
		{
			continue;
		}

		Transform* containerTransform = TransformAPI::findChildByName(canvas, "Container");
		if (!containerTransform)
		{
			continue;
		}

		Transform* backgroundTransform = TransformAPI::findChildByName(containerTransform, "Background");
		Transform* borderTransform = TransformAPI::findChildByName(containerTransform, "Border");
		Transform* glowTransform = TransformAPI::findChildByName(containerTransform, "Glow");

		if (!backgroundTransform || !borderTransform || !glowTransform)
		{
			continue;
		}

		GameObject* containerObject = ComponentAPI::getOwner(containerTransform);
		GameObject* backgroundObject = ComponentAPI::getOwner(backgroundTransform);
		GameObject* borderObject = ComponentAPI::getOwner(borderTransform);
		GameObject* glowObject = ComponentAPI::getOwner(glowTransform);

		if (!containerObject || !backgroundObject || !borderObject || !glowObject)
		{
			continue;
		}

		Transform2D* container = static_cast<Transform2D*>(GameObjectAPI::getComponent(containerObject, ComponentType::TRANSFORM2D));
		Transform2D* background = static_cast<Transform2D*>(GameObjectAPI::getComponent(backgroundObject, ComponentType::TRANSFORM2D));
		Transform2D* border = static_cast<Transform2D*>(GameObjectAPI::getComponent(borderObject, ComponentType::TRANSFORM2D));
		Transform2D* glow = static_cast<Transform2D*>(GameObjectAPI::getComponent(glowObject, ComponentType::TRANSFORM2D));

		if (!container || !background || !border || !glow)
		{
			continue;
		}

		RisenSpiresUISlot slot;
		slot.canvas = canvas;
		slot.container = container;
		slot.background = background;
		slot.border = border;
		slot.glow = glow;
		slot.active = false;

		m_risenSpiresUISlots.push_back(slot);
	}

	Debug::log("[AelorinUI] Cached %d Risen Spires UI slots", static_cast<int>(m_risenSpiresUISlots.size()));
}

void AelorinUI::setRisenSpiresSlotRadius(RisenSpiresUISlot& slot, float radius)
{
	if (!slot.container)
	{
		return;
	}

	const float baseDiameterUI = Transform2DAPI::getBaseSize(slot.container).x;
	if (baseDiameterUI <= 0.001f)
	{
		return;
	}

	const float desiredDiameterUI = radius * 2.0f * 100.0f;
	const float radiusScale = desiredDiameterUI / baseDiameterUI;

	Transform2DAPI::setScale(slot.container, Vector2(radiusScale, radiusScale));
}

void AelorinUI::updateRisenSpiresUI(float deltaTime)
{
	if (!m_risenSpiresUIActive)
	{
		return;
	}

	m_risenSpiresUITimer += deltaTime;

	// charge
	if (m_risenSpiresUITimer < m_risenSpiresUIChargeDuration)
	{
		const float t = std::clamp(m_risenSpiresUITimer / m_risenSpiresUIChargeDuration, 0.0f, 1.0f);
		const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);
		const float chargeScale = 0.1f + 0.9 * easedT;

		for (RisenSpiresUISlot& slot : m_risenSpiresUISlots)
		{
			if (!slot.active || !slot.background)
			{
				continue;
			}

			Transform2DAPI::setScale(slot.background, Vector2(chargeScale, chargeScale));
		}

		return;
	}

	// impact
	const float impactTimer = m_risenSpiresUITimer - m_risenSpiresUIChargeDuration;
	const float impactT = std::clamp(impactTimer / m_risenSpiresUIImpactFadeDuration, 0.0f, 1.0f);
	const float fadeAlpha = 1.0f - impactT;

	for (RisenSpiresUISlot& slot : m_risenSpiresUISlots)
	{
		if (!slot.active)
		{
			continue;
		}

		if (slot.background)
		{
			Transform2DAPI::setScale(slot.background, Vector2(1.0f, 1.0f));
		}

		if (slot.glow)
		{
			Transform2DAPI::setAlpha(slot.glow, fadeAlpha);
		}

		if (slot.container)
		{
			Transform2DAPI::setAlpha(slot.container, fadeAlpha);
		}
	}

	if (impactT >= 1.0f)
	{
		hideAllRisenSpiresUI();
	}
}

void AelorinUI::hideRisenSpiresUISlot(RisenSpiresUISlot& slot)
{
	if (slot.canvas)
	{
		GameObject* canvasObject = ComponentAPI::getOwner(slot.canvas);
		if (canvasObject)
		{
			GameObjectAPI::setActive(canvasObject, false);
		}
	}

	slot.active = false;
}

AelorinUI::RisenSpiresUISlot* AelorinUI::acquireRisenSpiresUISlot()
{
	for (RisenSpiresUISlot& slot : m_risenSpiresUISlots)
	{
		if (!slot.active)
		{
			return &slot;
		}
	}

	return nullptr;
}

void AelorinUI::updateSpiritCannonUI(float deltaTime)
{
	if (!m_spiritCannonUIActive)
	{
		return;
	}

	if (!m_spiritCannonOriginTransform ||
		!m_spiritCannonTargetTransform ||
		!m_spiritCannonUICanvasTransform ||
		!m_spiritCannonUIBackgroundTransform2D ||
		!m_spiritCannonUIGlowTransform2D)
	{
		hideSpiritCannonUI();
		return;
	}

	const Vector3 origin = TransformAPI::getGlobalPosition(m_spiritCannonOriginTransform);
	const Vector3 targetPosition = TransformAPI::getGlobalPosition(m_spiritCannonTargetTransform);

	Vector3 direction =	targetPosition - origin;
	direction.y = 0.0f;

	if (direction.LengthSquared() <= 0.00001f)
	{
		return;
	}

	direction.Normalize();

	Vector3 uiPosition = origin + direction * (m_spiritCannonBeamLength * 0.5f);
	uiPosition.y += 0.05f;

	TransformAPI::setGlobalPosition(m_spiritCannonUICanvasTransform, uiPosition);

	constexpr float radiansToDegrees = 180.0f / 3.14159265f;
	const float angleDegrees = std::atan2(direction.z, direction.x) * radiansToDegrees;

	TransformAPI::setGlobalRotationEuler(m_spiritCannonUICanvasTransform, Vector3(90.0f, 0.0f, angleDegrees));

	// charge
	if (m_spiritCannonUICharging)
	{
		m_spiritCannonUITimer += deltaTime;

		const float t = std::clamp(m_spiritCannonUITimer / m_spiritCannonUIChargeDuration, 0.0f, 1.0f);
		const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);

		Transform2DAPI::setAlpha(m_spiritCannonUIBackgroundTransform2D, easedT);

		if (t >= 1.0f)
		{
			m_spiritCannonUICharging = false;
			playSpiritCannonImpactUI();
		}
	}

	// impact
	if (m_spiritCannonImpactUIPlaying)
	{
		m_spiritCannonImpactUITimer += deltaTime;

		const float impactT = std::clamp(m_spiritCannonImpactUITimer / m_spiritCannonUIImpactFadeDuration, 0.0f, 1.0f);
		const float fadeAlpha =	1.0f - MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, impactT);

		Transform2DAPI::setAlpha(m_spiritCannonUIGlowTransform2D, fadeAlpha);

		if (impactT >= 1.0f)
		{
			m_spiritCannonImpactUIPlaying = false;
			m_spiritCannonImpactUITimer = 0.0f;

			Transform2DAPI::setAlpha(m_spiritCannonUIGlowTransform2D, 0.0f);

			if (!m_spiritCannonUICharging)
			{
				hideSpiritCannonUI();
				return;
			}
		}
	}
}

void AelorinUI::playSpiritCannonImpactUI()
{
	if (!m_spiritCannonUIBackgroundTransform2D || !m_spiritCannonUIGlowTransform2D)
	{
		return;
	}

	// shot fired -> reset charge
	Transform2DAPI::setAlpha(m_spiritCannonUIBackgroundTransform2D, 0.0f);

	// flash
	m_spiritCannonImpactUIPlaying = true;
	m_spiritCannonImpactUITimer = 0.0f;

	Transform2DAPI::setAlpha(m_spiritCannonUIGlowTransform2D, 1.0f);
}

void AelorinUI::hideSpiritCannonUI()
{
	if (m_spiritCannonUICanvasTransform)
	{
		GameObject* canvasObject = ComponentAPI::getOwner(m_spiritCannonUICanvasTransform);
		if (canvasObject)
		{
			GameObjectAPI::setActive(canvasObject, false);
		}
	}

	m_spiritCannonOriginTransform = nullptr;
	m_spiritCannonTargetTransform = nullptr;
	m_spiritCannonUIActive = false;
	m_spiritCannonUICharging = false;
	m_spiritCannonImpactUIPlaying = false;
	m_spiritCannonUITimer = 0.0f;
	m_spiritCannonUIChargeDuration = 0.0f;
	m_spiritCannonBeamLength = 0.0f;
	m_spiritCannonBeamWidth = 0.0f;
	m_spiritCannonImpactUITimer = 0.0f;
}

void AelorinUI::setSpiritCannonSize(float beamLength, float beamWidth)
{
	if (!m_spiritCannonUIContainerTransform2D)
	{
		return;
	}

	Transform2DAPI::setScale(m_spiritCannonUIContainerTransform2D, Vector2(beamLength, beamWidth));
}

void AelorinUI::setGraspOfTheDeadRadius(float radius)
{
	if (!m_graspOfTheDeadUIContainerTransform2D)
	{
		return;
	}

	const float baseDiameterUI = Transform2DAPI::getBaseSize(m_graspOfTheDeadUIContainerTransform2D).x;

	if (baseDiameterUI <= 0.001f)
	{
		return;
	}

	const float desiredDiameterUI =	radius * 2.0f * 100.0f;
	const float scale =	desiredDiameterUI / baseDiameterUI;

	Transform2DAPI::setScale(m_graspOfTheDeadUIContainerTransform2D, Vector2(scale, scale));
}

void AelorinUI::updateGraspOfTheDeadUI(float deltaTime)
{
	if (!m_graspOfTheDeadUIActive)
	{
		return;
	}

	if (!m_graspOfTheDeadUIBackgroundTransform2D ||	!m_graspOfTheDeadUIGlowTransform2D)
	{
		hideGraspOfTheDeadUI();
		return;
	}

	m_graspOfTheDeadUITimer += deltaTime;

	const float t =	std::clamp(m_graspOfTheDeadUITimer / m_graspOfTheDeadUIDuration, 0.0f, 1.0f);
	const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);
	const float pullScale =	1.0f - 0.9f * easedT;

	Transform2DAPI::setScale(m_graspOfTheDeadUIBackgroundTransform2D, Vector2(pullScale, pullScale));
	Transform2DAPI::setScale(m_graspOfTheDeadUIBorderTransform2D, Vector2(pullScale, pullScale));
	Transform2DAPI::setScale(m_graspOfTheDeadUIGlowTransform2D, Vector2(pullScale, pullScale));
	Transform2DAPI::setAlpha(m_graspOfTheDeadUIGlowTransform2D,	easedT);

	if (t >= 1.0f)
	{
		hideGraspOfTheDeadUI();
	}
}

void AelorinUI::hideGraspOfTheDeadUI()
{
	if (m_graspOfTheDeadUICanvasTransform)
	{
		GameObject* canvasObject = ComponentAPI::getOwner(m_graspOfTheDeadUICanvasTransform);
		if (canvasObject)
		{
			GameObjectAPI::setActive(canvasObject, false);
		}
	}

	m_graspOfTheDeadUIActive = false;
	m_graspOfTheDeadUITimer = 0.0f;
	m_graspOfTheDeadUIDuration = 0.0f;

	if (m_graspOfTheDeadUIGlowTransform2D)
	{
		Transform2DAPI::setAlpha(m_graspOfTheDeadUIGlowTransform2D,	0.0f);
	}
}

IMPLEMENT_SCRIPT(AelorinUI)

void AelorinUI::setupSoulCataclysmSafeZonesUI()
{
	m_soulCataclysmSafeZoneUISlots.clear();

	if (!m_soulCataclysmSafeZoneUICanvasTransform ||
		!m_soulCataclysmSafeZoneUIContainerTransform2D ||
		!m_soulCataclysmSafeZoneUIBackgroundTransform2D ||
		!m_soulCataclysmSafeZoneUIBorderTransform2D ||
		!m_soulCataclysmSafeZoneUIGlowTransform2D)
	{
		Debug::warn("[AelorinUI] Soul Cataclysm Safe Zone template UI is incomplete");

		return;
	}

	// Slot01 - assigned in editor
	SoulCataclysmSafeZoneUISlot firstSlot;
	firstSlot.canvas = m_soulCataclysmSafeZoneUICanvasTransform;
	firstSlot.container = m_soulCataclysmSafeZoneUIContainerTransform2D;
	firstSlot.background = m_soulCataclysmSafeZoneUIBackgroundTransform2D;
	firstSlot.border = m_soulCataclysmSafeZoneUIBorderTransform2D;
	firstSlot.glow = m_soulCataclysmSafeZoneUIGlowTransform2D;

	m_soulCataclysmSafeZoneUISlots.push_back(firstSlot);

	// parent should be "SafeZones"
	Transform* safeZonesUIRoot = TransformAPI::getParent(m_soulCataclysmSafeZoneUICanvasTransform);
	if (!safeZonesUIRoot)
	{
		Debug::warn("[AelorinUI] Soul Cataclysm Safe Zones UI root not found");

		return;
	}

	const int childCount = TransformAPI::getChildCount(safeZonesUIRoot);

	for (int i = 0; i < childCount; ++i)
	{
		Transform* canvas = TransformAPI::getChild(safeZonesUIRoot, i);

		if (!canvas || canvas == m_soulCataclysmSafeZoneUICanvasTransform)
		{
			continue;
		}

		Transform* containerTransform = TransformAPI::findChildByName(canvas, "Container");
		if (!containerTransform)
		{
			continue;
		}

		Transform* backgroundTransform = TransformAPI::findChildByName(containerTransform, "Background");
		Transform* borderTransform = TransformAPI::findChildByName(containerTransform, "Border");
		Transform* glowTransform = TransformAPI::findChildByName(containerTransform, "Glow");

		if (!backgroundTransform ||
			!borderTransform ||
			!glowTransform)
		{
			continue;
		}

		GameObject* containerObject = ComponentAPI::getOwner(containerTransform);

		GameObject* backgroundObject = ComponentAPI::getOwner(backgroundTransform);

		GameObject* borderObject = ComponentAPI::getOwner(borderTransform);

		GameObject* glowObject = ComponentAPI::getOwner(glowTransform);

		if (!containerObject ||
			!backgroundObject ||
			!borderObject ||
			!glowObject)
		{
			continue;
		}

		Transform2D* container = static_cast<Transform2D*>(GameObjectAPI::getComponent(containerObject, ComponentType::TRANSFORM2D));
		Transform2D* background = static_cast<Transform2D*>(GameObjectAPI::getComponent(backgroundObject, ComponentType::TRANSFORM2D));
		Transform2D* border = static_cast<Transform2D*>(GameObjectAPI::getComponent(borderObject, ComponentType::TRANSFORM2D));
		Transform2D* glow =	static_cast<Transform2D*>(GameObjectAPI::getComponent(glowObject, ComponentType::TRANSFORM2D));

		if (!container ||
			!background ||
			!border ||
			!glow)
		{
			continue;
		}

		SoulCataclysmSafeZoneUISlot slot;

		slot.canvas = canvas;
		slot.container = container;
		slot.background = background;
		slot.border = border;
		slot.glow = glow;

		m_soulCataclysmSafeZoneUISlots.push_back(slot);
	}

	Debug::log("[AelorinUI] Cached %d Soul Cataclysm Safe Zone UI slots",static_cast<int>(m_soulCataclysmSafeZoneUISlots.size()));
}

void AelorinUI::hideSoulCataclysmUI()
{
	if (m_soulCataclysmUICanvasTransform)
	{
		GameObject* arenaObject = ComponentAPI::getOwner(m_soulCataclysmUICanvasTransform);
		if (arenaObject)
		{
			GameObjectAPI::setActive(arenaObject, false);
		}
	}

	hideAllSoulCataclysmSafeZonesUI();

	m_soulCataclysmUIActive = false;
	m_soulCataclysmUITimer = 0.0f;
	m_soulCataclysmUIChannelDuration = 0.0f;

	if (m_soulCataclysmUIGlowTransform2D)
	{
		Transform2DAPI::setAlpha(m_soulCataclysmUIGlowTransform2D, 0.0f);
	}
}

void AelorinUI::hideAllSoulCataclysmSafeZonesUI()
{
	for (SoulCataclysmSafeZoneUISlot& slot : m_soulCataclysmSafeZoneUISlots)
	{
		hideSoulCataclysmSafeZoneUISlot(slot);
	}
}

void AelorinUI::setSoulCataclysmArenaRadius(float radius)
{
	if (!m_soulCataclysmUIContainerTransform2D)
	{
		return;
	}

	const float baseDiameterUI = Transform2DAPI::getBaseSize(m_soulCataclysmUIContainerTransform2D).x;
	if (baseDiameterUI <= 0.001f)
	{
		return;
	}

	const float desiredDiameterUI = radius * 2.0f * 100.0f;
	const float scale = desiredDiameterUI / baseDiameterUI;

	Transform2DAPI::setScale(m_soulCataclysmUIContainerTransform2D, Vector2(scale, scale));
}

void AelorinUI::setSoulCataclysmSafeZoneRadius(SoulCataclysmSafeZoneUISlot& slot, float radius)
{
	if (!slot.container)
	{
		return;
	}

	const float baseDiameterUI = Transform2DAPI::getBaseSize(slot.container).x;
	if (baseDiameterUI <= 0.001f)
	{
		return;
	}

	const float desiredDiameterUI = radius * 2.0f * 100.0f;
	const float scale = desiredDiameterUI / baseDiameterUI;

	Transform2DAPI::setScale(slot.container, Vector2(scale, scale));
}

void AelorinUI::updateSoulCataclysmUI(float deltaTime)
{
	if (!m_soulCataclysmUIActive)
	{
		return;
	}

	if (!m_soulCataclysmUIBackgroundTransform2D || !m_soulCataclysmUIGlowTransform2D)
	{
		hideSoulCataclysmUI();
		return;
	}

	m_soulCataclysmUITimer += deltaTime;

	const float t = std::clamp(m_soulCataclysmUITimer / m_soulCataclysmUIChannelDuration, 0.0f, 1.0f);
	const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);

	// stronger danger area as cataclysm nears execution
	const float backgroundAlpha = 0.25f + 0.55f * easedT;

	Transform2DAPI::setAlpha(m_soulCataclysmUIBackgroundTransform2D, backgroundAlpha);

	// impact
	Transform2DAPI::setAlpha(m_soulCataclysmUIGlowTransform2D, easedT);

	if (t >= 1.0f)
	{
		hideSoulCataclysmUI();
	}
}

void AelorinUI::hideSoulCataclysmSafeZoneUISlot(SoulCataclysmSafeZoneUISlot& slot)
{
	if (slot.canvas)
	{
		GameObject* slotObject = ComponentAPI::getOwner(slot.canvas);
		if (slotObject)
		{
			GameObjectAPI::setActive(slotObject, false);
		}
	}

	slot.active = false;
}

AelorinUI::SoulCataclysmSafeZoneUISlot* AelorinUI::acquireSoulCataclysmSafeZoneUISlot()
{
	for (SoulCataclysmSafeZoneUISlot& slot :m_soulCataclysmSafeZoneUISlots)
	{
		if (!slot.active)
		{
			return &slot;
		}
	}

	return nullptr;
}
