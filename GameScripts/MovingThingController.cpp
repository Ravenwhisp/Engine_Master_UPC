#include "pch.h"
#include "MovingThingController.h"

MovingThingController::MovingThingController(GameObject* owner)
    : Script(owner)
{
}

void MovingThingController::Start()
{
}

void MovingThingController::Update()
{
}

void MovingThingController::drawGizmo()
{

	const Vector3 white = { 1.0f, 1.0f, 1.0f };
	const Vector3 red = { 1.0f, 0.0f, 0.0f };
	const Vector3 yellow = { 1.0f, 1.0f, 0.0f };
	const Vector3 cyan = { 0.0f, 1.0f, 1.0f };

	GameObject* owner = getOwner();
	Vector3 position = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(owner));

	if (m_isOnIdle)
	{
		Vector3 debugPosition = position + Vector3(0.0f, 0.2f, 0.0f);

		DebugDrawAPI::drawCircle(debugPosition, Vector3(0.0f, 1.0f, 0.0f), white, m_detectionRadius, 24.0f, 0, true);
	
	}
	else 
	{
		Vector3 debugPosition = position + Vector3(0.0f, 0.2f, 0.0f);
		Vector3 fleeDirection3D = Vector3(m_fleeDirection.x, 0.0f, m_fleeDirection.y);
		Vector3 fleeTargetPosition = debugPosition + fleeDirection3D * m_detectionRadius;
		
		DebugDrawAPI::drawLine(debugPosition, fleeTargetPosition, red, 0, true);
	}
}

IMPLEMENT_SCRIPT_FIELDS(MovingThingController,
    SERIALIZED_FLOAT(m_detectionRadius, "Detection Radius", 0.01f, 100.0f, 0.01f),
    SERIALIZED_FLOAT(m_fleeSpeed, "Flee Speed", 0.0f, 100.0f, 0.01f),
	SERIALIZED_ASSET_REF(m_disappearParticlesPrefab, "Disappear Particles Prefab", AssetType::PREFAB),
    SERIALIZED_COMPONENT_REF(m_lyrielTransform, "Lyriel Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_deathTransform, "Death Transform", ComponentType::TRANSFORM)
)

IMPLEMENT_SCRIPT(MovingThingController)