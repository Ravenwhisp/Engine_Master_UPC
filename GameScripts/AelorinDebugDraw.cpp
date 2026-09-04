#include "pch.h"
#include "AelorinDebugDraw.h"

#include "AelorinAttackConfig.h"
#include "AelorinBossController.h"
#include "AelorinSummonSlot.h"

IMPLEMENT_SCRIPT_FIELDS(AelorinDebugDraw,
	FIELD_GROUP_COLLAPSE("Seeker Sigils",
		SERIALIZED_BOOL(m_drawSeekerSigilsNormal, "Draw Normal Impact Areas"),
		SERIALIZED_BOOL(m_drawSeekerSigilsLarge, "Draw Large Impact Areas")
	),

	FIELD_GROUP_COLLAPSE("Nova",
		SERIALIZED_BOOL(m_drawNova, "Draw Nova")
	),	

	FIELD_GROUP_COLLAPSE("Risen Spires",
		SERIALIZED_BOOL(m_drawRisenSpiresPatternA, "Draw Pattern A"),
		SERIALIZED_BOOL(m_drawRisenSpiresPatternB, "Draw Pattern B")
	),

	FIELD_GROUP_COLLAPSE("Spirit Cannon",
		SERIALIZED_BOOL(m_drawSpiritCannon, "Draw Spirit Cannon")
	),

	FIELD_GROUP_COLLAPSE("Grasp of the Dead",
		SERIALIZED_BOOL(m_drawGraspCenter, "Draw Grasp Center"),
		SERIALIZED_BOOL(m_drawGraspNova, "Draw Grasp Nova")
	),

	FIELD_GROUP_COLLAPSE("Teleport",
		SERIALIZED_BOOL(m_drawTeleport, "Draw Teleport")
	),

	FIELD_GROUP_COLLAPSE("Summon",
		SERIALIZED_BOOL(m_drawPhase1SummonSlots, "Draw Phase 1 Summon Slots"),
		SERIALIZED_BOOL(m_drawPhase2SummonSlots, "Draw Phase 2 Summon Slots")
	),

	FIELD_GROUP_COLLAPSE("Soul Cataclysm",
		SERIALIZED_BOOL(m_drawSoulCataclysmSafeZones, "Draw Safe Zones"),
		SERIALIZED_BOOL(m_drawSoulCataclysmRadius, "Draw Cataclysm Radius")
	),

	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled"),
	SERIALIZED_FLOAT(m_heightOffset, "Height Offset", 0.0f, 5.0f, 0.05f)
)


AelorinDebugDraw::AelorinDebugDraw(GameObject* owner)
	: Script(owner)
{
}

void AelorinDebugDraw::Start()
{
	m_controller = GameObjectAPI::findScript<AelorinBossController>(getOwner());

	if (!m_controller)
	{
		Debug::warn("[AelorinDebugDraw] AelorinBossController not found.");
	}
}

void AelorinDebugDraw::drawGizmo()
{
	if (!m_debugEnabled)
	{
		return;
	}

	if (!m_controller)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();

	if (!config)
	{
		return;
	}

	const Vector3 red = Vector3(1.0f, 0.0f, 0.0f);
	const Vector3 cyan = Vector3(0.0f, 1.0f, 1.0f);
	const Vector3 orange = Vector3(1.0f, 0.5f, 0.0f);
	const Vector3 yellow = Vector3(1.0f, 1.0f, 0.0f);
	const Vector3 green = Vector3(0.0f, 1.0f, 0.0f);

	if (m_drawSeekerSigilsNormal)
	{
		drawImpactCircle(m_controller->getLyrielPosition(), config->m_seekerSigilsRadius, cyan);
		drawImpactCircle(m_controller->getDeathPosition(), config->m_seekerSigilsRadius, cyan);
	}

	if (m_drawSeekerSigilsLarge)
	{
		const Vector3 lyrielPosition = m_controller->getLyrielPosition();
		const Vector3 deathPosition = m_controller->getDeathPosition();
		const Vector3 midpoint = (lyrielPosition + deathPosition) * 0.5f;

		drawImpactCircle(midpoint, config->m_seekerSigilsPhase2FinalRadius, red);
	}

	if (m_drawNova)
	{
		Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
		if (!ownerTransform)
		{
			return;
		}

		const Vector3 bossPosition = TransformAPI::getGlobalPosition(ownerTransform);
		drawImpactCircle(bossPosition, config->m_novaTriggerDistance, red); // trigger distance
		drawImpactCircle(bossPosition, config->m_novaRadius, cyan); // first wave radius
		drawImpactCircle(bossPosition, config->m_novaPhase2SecondRadius, orange); // second wave radius
	}

	if (m_drawRisenSpiresPatternA)
	{
		Transform* patternARoot = m_controller->getRisenSpiresPatternARoot();
		if (patternARoot)
		{
			const int childCount = TransformAPI::getChildCount(patternARoot);

			for (int i = 0; i < childCount; ++i)
			{
				Transform* point = TransformAPI::getChild(patternARoot, i);
				if (!point)
				{
					continue;
				}

				const Vector3 position = TransformAPI::getGlobalPosition(point);
				drawImpactCircle(position, config->m_risenSpiresRadius, cyan);
			}
		}
	}

	if (m_drawRisenSpiresPatternB)
	{
		Transform* patternBRoot = m_controller->getRisenSpiresPatternBRoot();
		if (patternBRoot)
		{
			const int childCount = TransformAPI::getChildCount(patternBRoot);

			for (int i = 0; i < childCount; ++i)
			{
				Transform* point = TransformAPI::getChild(patternBRoot, i);
				if (!point)
				{
					continue;
				}

				const Vector3 position = TransformAPI::getGlobalPosition(point);
				drawImpactCircle(position, config->m_risenSpiresRadius, orange);
			}
		}
	}

	if (m_drawSpiritCannon && m_controller->hasSpiritCannonDebugLine())
	{
		drawBeam(
			m_controller->getSpiritCannonDebugOrigin(),
			m_controller->getSpiritCannonDebugDirection(),
			config->m_spiritCannonBeamLength,
			m_controller->getSpiritCannonDebugWidth(),
			yellow
		);
	}

	if (m_drawGraspCenter)
	{
		Transform* graspCenter = m_controller->getGraspCenter();
		if (graspCenter)
		{
			Vector3 centerPosition = TransformAPI::getGlobalPosition(graspCenter);
			centerPosition.y += m_heightOffset;

			DebugDrawAPI::drawCross(
				centerPosition,
				0.5f,
				0,
				true
			);
		}
	}

	if (m_drawGraspNova)
	{
		Transform* graspCenter = m_controller->getGraspCenter();
		if (graspCenter)
		{
			Vector3 centerPosition = TransformAPI::getGlobalPosition(graspCenter);
			centerPosition.y += m_heightOffset;

			drawImpactCircle(
				centerPosition,
				config->m_novaRadius,
				cyan
			);

			drawImpactCircle(
				centerPosition,
				config->m_novaPhase2SecondRadius,
				orange
			);
		}
	}

	if (m_drawTeleport)
	{
		Transform* anchorsRoot = m_controller->getTeleportAnchorsRoot();
		if (anchorsRoot)
		{
			const int childCount = TransformAPI::getChildCount(anchorsRoot);

			for (int i = 0; i < childCount; ++i)
			{
				Transform* anchor = TransformAPI::getChild(anchorsRoot, i);
				if (!anchor)
				{
					continue;
				}

				const Vector3 anchorPosition = TransformAPI::getGlobalPosition(anchor);

				drawImpactCircle(anchorPosition, config->m_teleportTriggerDistance, yellow);
			}
		}
	}

	if (m_drawPhase1SummonSlots)
	{
		drawSummonSlots(m_controller->getPhase1SummonFormation(), cyan);
	}

	if (m_drawPhase2SummonSlots)
	{
		drawSummonSlots(m_controller->getPhase2SummonFormation(), orange);
	}

	if (m_drawSoulCataclysmSafeZones)
	{
		Transform* safeZonesRoot = m_controller->getTeleportAnchorsRoot();
		if (safeZonesRoot)
		{
			const int childCount = TransformAPI::getChildCount(safeZonesRoot);

			for (int i = 0; i < childCount; ++i)
			{
				Transform* safeZone = TransformAPI::getChild(safeZonesRoot, i);
				if (!safeZone)
				{
					continue;
				}

				const Vector3 position = TransformAPI::getGlobalPosition(safeZone);

				drawImpactCircle(position, config->m_soulCataclysmSafeZoneRadius, green);
			}
		}
	}

	if (m_drawSoulCataclysmRadius)
	{
		Transform* cataclysmCenter = m_controller->getSoulCataclysmCenter();
		if (cataclysmCenter)
		{
			Vector3 centerPosition = TransformAPI::getGlobalPosition(cataclysmCenter);
			centerPosition.y += m_heightOffset;

			drawImpactCircle(
				centerPosition,
				config->m_soulCataclysmRadius,
				red
			);
		}
	}
}

void AelorinDebugDraw::drawImpactCircle(const Vector3& position, float radius, const Vector3& color) const
{
	Vector3 debugPosition = position;
	debugPosition.y += m_heightOffset;

	DebugDrawAPI::drawCircle(
		debugPosition,
		Vector3(0.0f, 1.0f, 0.0f),
		color,
		radius,
		32.0f,
		0,
		true
	);

	DebugDrawAPI::drawPoint(debugPosition, color, 8.0f, 0,true);
}

void AelorinDebugDraw::drawBeam(const Vector3& origin, const Vector3& direction, float length, float width, const Vector3& color)
{
	Vector3 forward = direction;
	forward.y = 0.0f;

	if (forward.LengthSquared() <= 0.00001f)
	{
		return;
	}

	forward.Normalize();

	// Perpendicular horizontal direction
	Vector3 right(forward.z, 0.0f, -forward.x);
	right.Normalize();

	const float halfWidth = width * 0.5f;
	const Vector3 startLeft = origin - right * halfWidth;
	const Vector3 startRight = origin + right * halfWidth;
	const Vector3 endCenter = origin + forward * length;
	const Vector3 endLeft = endCenter - right * halfWidth;
	const Vector3 endRight = endCenter + right * halfWidth;

	// Rectangle borders
	DebugDrawAPI::drawLine(startLeft, endLeft, color, 0, true);
	DebugDrawAPI::drawLine(startRight, endRight, color, 0, true);
	DebugDrawAPI::drawLine(startLeft, startRight, color, 0, true);
	DebugDrawAPI::drawLine(endLeft, endRight, color, 0, true);

	// Center firing line
	DebugDrawAPI::drawArrow(origin, endCenter, color, 0.25f, 0, true);
}

void AelorinDebugDraw::drawSummonSlots(Transform* formationRoot, const Vector3& freeColor) const
{
	if (!formationRoot)
	{
		return;
	}

	const int childCount =
		TransformAPI::getChildCount(
			formationRoot
		);

	for (int i = 0; i < childCount; ++i)
	{
		Transform* slotTransform = TransformAPI::getChild(formationRoot, i);
		if (!slotTransform)
		{
			continue;
		}

		GameObject* slotObject = ComponentAPI::getOwner(slotTransform);
		if (!slotObject)
		{
			continue;
		}

		AelorinSummonSlot* slot = GameObjectAPI::findScript<AelorinSummonSlot>(slotObject);
		if (!slot)
		{
			continue;
		}

		const Vector3 position = TransformAPI::getGlobalPosition(slotTransform);
		const bool occupied = slot->hasLivingEnemy();
		const Vector3 color = occupied ? Vector3(1.0f, 0.0f, 0.0f) : freeColor;

		drawImpactCircle(position, 0.6f, color);
	}
}

IMPLEMENT_SCRIPT(AelorinDebugDraw)