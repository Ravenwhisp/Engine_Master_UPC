#include "pch.h"
#include "PlayerRotation.h"

#include <cmath>

static const float PI = 3.1415926535897931f;

IMPLEMENT_SCRIPT_FIELDS(PlayerRotation,
	SERIALIZED_FLOAT(m_turnSpeedDegPerSec, "Turn Speed (deg/s)", 0.0f, 2000.0f, 1.0f)
)

PlayerRotation::PlayerRotation(GameObject* owner)
	: Script(owner)
{
}

void PlayerRotation::Start()
{
	GameObject* owner = getOwner();
}

void PlayerRotation::Update()
{
}

void PlayerRotation::onAfterDeserialize()
{
	m_yawInitialized = false;
	m_currentYawDeg = 0.0f;
}

void PlayerRotation::applyFacingFromDirection(GameObject* owner, const Vector3& direction, float dt)
{
	const float yawRad = std::atan2(direction.x, direction.z);
	const float targetYawDeg = yawRad * (180.0f / PI);

	Transform* transform = GameObjectAPI::getTransform(owner);
	Vector3 currentEuler = TransformAPI::getEulerDegrees(transform);
	const float currentWorldYaw = currentEuler.y;

	const float maxStep = m_turnSpeedDegPerSec * dt;
	const float newYaw = moveTowardsAngleDegrees(currentWorldYaw, targetYawDeg, maxStep);

	TransformAPI::setRotationEuler(transform, Vector3(currentEuler.x, newYaw, currentEuler.z));
}

float PlayerRotation::wrapAngleDegrees(float angle)
{
	while (angle > 180.0f)
	{
		angle -= 360.0f;
	}
	while (angle < -180.0f)
	{
		angle += 360.0f;
	}
	return angle;
}

float PlayerRotation::moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta)
{
	float delta = wrapAngleDegrees(targetYawAngle - currentYawAngle);

	if (delta > maxDelta)
	{
		delta = maxDelta;
	}

	if (delta < -maxDelta)
	{
		delta = -maxDelta;
	}

	return currentYawAngle + delta;
}

IMPLEMENT_SCRIPT(PlayerRotation)