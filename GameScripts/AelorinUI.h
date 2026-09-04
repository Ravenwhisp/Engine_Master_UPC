#pragma once

#include "ScriptAPI.h"

#include <vector>

class Transform2D;

class AelorinUI : public Script
{
	DECLARE_SCRIPT(AelorinUI)

public:
	explicit AelorinUI(GameObject* owner);

	void Start() override;
	void Update() override;

	FieldList getExposedFields() const override;

	// Seeker Sigils
	void showSeekerSigilsUI(const Vector3& impactPosition, float radius, float telegraphDuration);

	// Nova
	void showNovaUI(const Vector3& center, float firstRadius, float firstChargeDuration, bool hasSecondWave, float secondRadius = 0.0f, float secondChargeDuration = 0.0f);

	// Risen Spires
	void showRisenSpiresUI(Transform* patternRoot, float radius, float chargeDuration);

	// Spirit Cannon
	void showSpiritCannonUI(Transform* originTransform, Transform* targetTransform, float beamLength, float beamWidth, float chargeDuration);

	// Grasp of the Dead
	void showGraspOfTheDeadUI(const Vector3& center, float radius, float pullDuration);

	// Soul Cataclysm
	void showSoulCataclysmUI(const Vector3& center, float radius, Transform* safeZonesRoot, float safeZoneRadius, float channelDuration);

private:

	// Seeker Sigils
	struct SeekerSigilsUISlot
	{
		Transform* canvas = nullptr;
		Transform2D* container = nullptr;
		Transform2D* background = nullptr;
		Transform2D* border = nullptr;
		Transform2D* glow = nullptr;

		bool active = false;

		float timer = 0.0f;
		float duration = 0.0f;
	};

	void setupSeekerSigilsUI();
	void hideAllSeekerSigilsUI();
	void updateSeekerSigilsUI(float deltaTime);
	void hideSeekerSigilsUISlot(SeekerSigilsUISlot& slot);
	SeekerSigilsUISlot* acquireSeekerSigilsUISlot();

	// Nova
	void hideNovaUI();
	void setNovaContainerRadius(float radius);
	void updateNovaUI(float deltaTime);

	// Risen Spires
	struct RisenSpiresUISlot
	{
		Transform* canvas = nullptr;
		Transform2D* container = nullptr;
		Transform2D* background = nullptr;
		Transform2D* border = nullptr;
		Transform2D* glow = nullptr;

		bool active = false;
	};

	void hideAllRisenSpiresUI();
	void setupRisenSpiresUI();
	void setRisenSpiresSlotRadius(RisenSpiresUISlot& slot, float radius);
	void updateRisenSpiresUI(float deltaTime);
	void hideRisenSpiresUISlot(RisenSpiresUISlot& slot);
	RisenSpiresUISlot* acquireRisenSpiresUISlot();

	// Spirit Cannon
	void updateSpiritCannonUI(float deltaTime);
	void playSpiritCannonImpactUI();
	void hideSpiritCannonUI();
	void setSpiritCannonSize(float beamLength, float beamWidth);

	// Grasp of the Dead
	void setGraspOfTheDeadRadius(float radius);
	void updateGraspOfTheDeadUI(float deltaTime);
	void hideGraspOfTheDeadUI();

	// Soul Cataclysm
	struct SoulCataclysmSafeZoneUISlot
	{
		Transform* canvas = nullptr;
		Transform2D* container = nullptr;
		Transform2D* background = nullptr;
		Transform2D* border = nullptr;
		Transform2D* glow = nullptr;

		bool active = false;
	};

	void setupSoulCataclysmSafeZonesUI();
	void hideSoulCataclysmUI();
	void hideAllSoulCataclysmSafeZonesUI();
	void setSoulCataclysmArenaRadius(float radius);
	void setSoulCataclysmSafeZoneRadius(SoulCataclysmSafeZoneUISlot& slot, float radius);
	void updateSoulCataclysmUI(float deltaTime);
	void hideSoulCataclysmSafeZoneUISlot(SoulCataclysmSafeZoneUISlot& slot);
	SoulCataclysmSafeZoneUISlot* acquireSoulCataclysmSafeZoneUISlot();

private:

	// Seeker Sigils
	ComponentRef<Transform> m_seekerSigilsUICanvas;
	ComponentRef<Transform2D> m_seekerSigilsUIContainer;
	ComponentRef<Transform2D> m_seekerSigilsUIBackground;
	ComponentRef<Transform2D> m_seekerSigilsUIBorder;
	ComponentRef<Transform2D> m_seekerSigilsUIGlow;

	Transform* m_seekerSigilsUICanvasTransform = nullptr;
	Transform2D* m_seekerSigilsUIContainerTransform2D = nullptr;
	Transform2D* m_seekerSigilsUIBackgroundTransform2D = nullptr;
	Transform2D* m_seekerSigilsUIBorderTransform2D = nullptr;
	Transform2D* m_seekerSigilsUIGlowTransform2D = nullptr;

	std::vector<SeekerSigilsUISlot> m_seekerSigilsUISlots;

	static constexpr float m_seekerSigilsImpactFadeDuration = 0.15f;

	// Nova
	ComponentRef<Transform> m_novaUICanvas;
	ComponentRef<Transform2D> m_novaUIContainer;
	ComponentRef<Transform2D> m_novaUIBackground;
	ComponentRef<Transform2D> m_novaUIBorder;
	ComponentRef<Transform2D> m_novaUIGlow;

	Transform* m_novaUICanvasTransform = nullptr;
	Transform2D* m_novaUIContainerTransform2D = nullptr;
	Transform2D* m_novaUIBackgroundTransform2D = nullptr;
	Transform2D* m_novaUIBorderTransform2D = nullptr;
	Transform2D* m_novaUIGlowTransform2D = nullptr;

	bool m_novaUIActive = false;
	bool m_novaUIHasSecondWave = false;
	bool m_novaUISecondWaveStarted = false;

	float m_novaUITimer = 0.0f;

	float m_novaUIFirstChargeDuration = 0.0f;
	float m_novaUISecondChargeDuration = 0.0f;

	float m_novaUIFirstRadius = 0.0f;
	float m_novaUISecondRadius = 0.0f;

	static constexpr float m_novaUIImpactFadeDuration = 0.15f;

	// Risen Spires UI
	ComponentRef<Transform> m_risenSpiresUICanvas;
	ComponentRef<Transform2D> m_risenSpiresUIContainer;
	ComponentRef<Transform2D> m_risenSpiresUIBackground;
	ComponentRef<Transform2D> m_risenSpiresUIBorder;
	ComponentRef<Transform2D> m_risenSpiresUIGlow;

	Transform* m_risenSpiresUICanvasTransform = nullptr;
	Transform2D* m_risenSpiresUIContainerTransform2D = nullptr;
	Transform2D* m_risenSpiresUIBackgroundTransform2D = nullptr;
	Transform2D* m_risenSpiresUIBorderTransform2D = nullptr;
	Transform2D* m_risenSpiresUIGlowTransform2D = nullptr;

	std::vector<RisenSpiresUISlot> m_risenSpiresUISlots;

	bool m_risenSpiresUIActive = false;
	float m_risenSpiresUITimer = 0.0f;
	float m_risenSpiresUIChargeDuration = 0.0f;
	int m_risenSpiresUIActiveSlotCount = 0;

	static constexpr float m_risenSpiresUIImpactFadeDuration = 0.15f;

	// Spirit Cannon
	ComponentRef<Transform> m_spiritCannonUICanvas;
	ComponentRef<Transform2D> m_spiritCannonUIContainer;
	ComponentRef<Transform2D> m_spiritCannonUIBackground;
	ComponentRef<Transform2D> m_spiritCannonUIBorder;
	ComponentRef<Transform2D> m_spiritCannonUIGlow;

	Transform* m_spiritCannonUICanvasTransform = nullptr;
	Transform2D* m_spiritCannonUIContainerTransform2D = nullptr;
	Transform2D* m_spiritCannonUIBackgroundTransform2D = nullptr;
	Transform2D* m_spiritCannonUIBorderTransform2D = nullptr;
	Transform2D* m_spiritCannonUIGlowTransform2D = nullptr;

	Transform* m_spiritCannonOriginTransform = nullptr;
	Transform* m_spiritCannonTargetTransform = nullptr;

	bool m_spiritCannonUIActive = false;
	bool m_spiritCannonUICharging = false;
	bool m_spiritCannonImpactUIPlaying = false;
	float m_spiritCannonUITimer = 0.0f;
	float m_spiritCannonUIChargeDuration = 0.0f;
	float m_spiritCannonBeamLength = 0.0f;
	float m_spiritCannonBeamWidth = 0.0f;
	float m_spiritCannonImpactUITimer = 0.0f;

	static constexpr float m_spiritCannonUIImpactFadeDuration = 0.15f;

	// Grasp of the Dead
	ComponentRef<Transform> m_graspOfTheDeadUICanvas;
	ComponentRef<Transform2D> m_graspOfTheDeadUIContainer;
	ComponentRef<Transform2D> m_graspOfTheDeadUIBackground;
	ComponentRef<Transform2D> m_graspOfTheDeadUIBorder;
	ComponentRef<Transform2D> m_graspOfTheDeadUIGlow;

	Transform* m_graspOfTheDeadUICanvasTransform = nullptr;
	Transform2D* m_graspOfTheDeadUIContainerTransform2D = nullptr;
	Transform2D* m_graspOfTheDeadUIBackgroundTransform2D = nullptr;
	Transform2D* m_graspOfTheDeadUIBorderTransform2D = nullptr;
	Transform2D* m_graspOfTheDeadUIGlowTransform2D = nullptr;

	bool m_graspOfTheDeadUIActive = false;

	float m_graspOfTheDeadUITimer = 0.0f;
	float m_graspOfTheDeadUIDuration = 0.0f;

	// Soul Cataclysm - Arena
	ComponentRef<Transform> m_soulCataclysmUICanvas;
	ComponentRef<Transform2D> m_soulCataclysmUIContainer;
	ComponentRef<Transform2D> m_soulCataclysmUIBackground;
	ComponentRef<Transform2D> m_soulCataclysmUIBorder;
	ComponentRef<Transform2D> m_soulCataclysmUIGlow;

	Transform* m_soulCataclysmUICanvasTransform = nullptr;
	Transform2D* m_soulCataclysmUIContainerTransform2D = nullptr;
	Transform2D* m_soulCataclysmUIBackgroundTransform2D = nullptr;
	Transform2D* m_soulCataclysmUIBorderTransform2D = nullptr;
	Transform2D* m_soulCataclysmUIGlowTransform2D = nullptr;

	// Soul Cataclysm - Safe Zone
	ComponentRef<Transform> m_soulCataclysmSafeZoneUICanvas;
	ComponentRef<Transform2D> m_soulCataclysmSafeZoneUIContainer;
	ComponentRef<Transform2D> m_soulCataclysmSafeZoneUIBackground;
	ComponentRef<Transform2D> m_soulCataclysmSafeZoneUIBorder;
	ComponentRef<Transform2D> m_soulCataclysmSafeZoneUIGlow;

	Transform* m_soulCataclysmSafeZoneUICanvasTransform = nullptr;
	Transform2D* m_soulCataclysmSafeZoneUIContainerTransform2D = nullptr;
	Transform2D* m_soulCataclysmSafeZoneUIBackgroundTransform2D = nullptr;
	Transform2D* m_soulCataclysmSafeZoneUIBorderTransform2D = nullptr;
	Transform2D* m_soulCataclysmSafeZoneUIGlowTransform2D = nullptr;

	std::vector<SoulCataclysmSafeZoneUISlot>m_soulCataclysmSafeZoneUISlots;

	bool m_soulCataclysmUIActive = false;
	float m_soulCataclysmUITimer = 0.0f;
	float m_soulCataclysmUIChannelDuration = 0.0f;
};