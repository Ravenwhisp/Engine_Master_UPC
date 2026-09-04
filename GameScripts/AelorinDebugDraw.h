#pragma once

#include "ScriptAPI.h"

class AelorinAttackConfig;
class AelorinBossController;

class AelorinDebugDraw : public Script
{
	DECLARE_SCRIPT(AelorinDebugDraw)

public:
	explicit AelorinDebugDraw(GameObject* owner);

	void Start() override;
	void drawGizmo() override;

	FieldList getExposedFields() const override;

public:
	bool m_debugEnabled = true;

	// Seeker Sigils
	bool m_drawSeekerSigilsNormal = true;
	bool m_drawSeekerSigilsLarge = true;

	// Nova
	bool m_drawNova = true;

	// Risen Spires
	bool m_drawRisenSpiresPatternA = true;
	bool m_drawRisenSpiresPatternB = true;

	// Spirit Cannon
	bool m_drawSpiritCannon = true;

	// Grasp of the Dead
	bool m_drawGraspCenter = true;
	bool m_drawGraspNova = true;

	// Teleport
	bool m_drawTeleport = true;

	// Summon
	bool m_drawPhase1SummonSlots = true;
	bool m_drawPhase2SummonSlots = true;

	// Soul Cataclysm
	bool m_drawSoulCataclysmSafeZones = true;
	bool m_drawSoulCataclysmRadius = true;

	float m_heightOffset = 0.15f;

private:
	AssetReference<AelorinAttackConfig> m_attackConfig;
	AelorinBossController* m_controller = nullptr;

private:
	void drawImpactCircle(const Vector3& position, float radius, const Vector3& color) const;
	void drawBeam(const Vector3& origin, const Vector3& direction, float length, float width, const Vector3& color);

	// Ability draw helpers
	void drawSummonSlots(Transform* formationRoot, const Vector3& freeColor) const;
};