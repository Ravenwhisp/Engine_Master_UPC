#include "pch.h"
#include "SkeletonAttackDebugDraw.h"

#include "SkeletonEnemyController.h"
#include "SkeletonAttackConfig.h"

IMPLEMENT_SCRIPT_FIELDS(SkeletonAttackDebugDraw,
	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled"),
	SERIALIZED_BOOL(m_drawScimitarStartRange, "Draw Scimitar Start Range"),
	SERIALIZED_BOOL(m_drawDashStopRange, "Draw Dash Stop Range"),
	SERIALIZED_BOOL(m_drawScimitarAttackArea, "Draw Attack Area"),
	SERIALIZED_BOOL(m_drawScimitarStunArea, "Draw Scimitar Stun Area"),
	SERIALIZED_FLOAT(m_heightOffset, "Height Offset", 0.0f, 5.0f, 0.05f)
)

SkeletonAttackDebugDraw::SkeletonAttackDebugDraw(GameObject* owner)
	: Script(owner)
{
}

void SkeletonAttackDebugDraw::Start()
{
	m_controller = GameObjectAPI::findScript<SkeletonEnemyController>(getOwner());
}

void SkeletonAttackDebugDraw::drawGizmo()
{
	if (!m_debugEnabled)
	{
		return;
	}

	// Gizmos may run before Start, so resolve here too.
	if (!m_controller)
	{
		m_controller = GameObjectAPI::findScript<SkeletonEnemyController>(getOwner());
	}

	const SkeletonAttackConfig* attackConfig = getAttackConfig();
	if (!attackConfig)
	{
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	Vector3 position = TransformAPI::getGlobalPosition(ownerTransform);
	position.y += m_heightOffset;

	const Vector3 up(0.0f, 1.0f, 0.0f);

	const Vector3 startRangeColor(0.0f, 1.0f, 1.0f);
	const Vector3 dashStopColor(1.0f, 1.0f, 0.0f);

	if (m_drawScimitarStartRange)
	{
		DebugDrawAPI::drawCircle(
			position,
			up,
			startRangeColor,
			attackConfig->m_scimitarStartRange,
			32.0f,
			0,
			true
		);
	}

	if (m_drawDashStopRange)
	{
		DebugDrawAPI::drawCircle(
			position,
			up,
			dashStopColor,
			attackConfig->m_scimitarDashStopRange,
			32.0f,
			0,
			true
		);
	}

	if (m_drawScimitarAttackArea)
	{
		drawScimitarAttackCone();
	}

	if (m_drawScimitarStunArea)
	{
		drawScimitarStunCone();
	}
}

const SkeletonAttackConfig* SkeletonAttackDebugDraw::getAttackConfig() const
{
	if (!m_controller)
	{
		return nullptr;
	}

	return m_controller->m_attackConfig.get();
}

void SkeletonAttackDebugDraw::drawScimitarAttackCone() const
{
	const SkeletonAttackConfig* attackConfig = getAttackConfig();
	if (!attackConfig)
	{
		return;
	}

	drawScimitarCone(
		attackConfig->m_basicAttackRange,
		attackConfig->m_scimitarHalfAngleDegrees,
		Vector3(1.0f, 0.0f, 0.0f)
	);
}

void SkeletonAttackDebugDraw::drawScimitarStunCone() const
{
	const SkeletonAttackConfig* attackConfig = getAttackConfig();
	if (!attackConfig)
	{
		return;
	}

	drawScimitarCone(
		attackConfig->m_scimitarStunHitRange,
		attackConfig->m_scimitarHalfAngleDegrees,
		Vector3(0.0f, 0.0f, 1.0f)
	);
}

void SkeletonAttackDebugDraw::drawScimitarCone(float range, float halfAngleDegrees, const Vector3& color) const
{
	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	ownerPosition.y += m_heightOffset;

	Vector3 forward = TransformAPI::getForward(ownerTransform);
	forward.y = 0.0f;

	if (forward.LengthSquared() < 0.0001f)
	{
		return;
	}

	forward.Normalize();

	constexpr float degreesToRadians = 3.14159265f / 180.0f;
	const float halfAngleRadians = halfAngleDegrees * degreesToRadians;

	const Vector3 leftDirection = rotateAroundY(forward, -halfAngleRadians);
	const Vector3 rightDirection = rotateAroundY(forward, halfAngleRadians);

	const Vector3 leftPoint = ownerPosition + leftDirection * range;
	const Vector3 rightPoint = ownerPosition + rightDirection * range;
	const Vector3 forwardPoint = ownerPosition + forward * range;

	DebugDrawAPI::drawLine(ownerPosition, leftPoint, color, 0, true);
	DebugDrawAPI::drawLine(ownerPosition, rightPoint, color, 0, true);
	DebugDrawAPI::drawLine(ownerPosition, forwardPoint, color, 0, true);

	constexpr int arcSegments = 24;

	Vector3 previousPoint = leftPoint;

	for (int i = 1; i <= arcSegments; ++i)
	{
		const float t = static_cast<float>(i) / static_cast<float>(arcSegments);
		const float angle = -halfAngleRadians + halfAngleRadians * 2.0f * t;

		const Vector3 direction = rotateAroundY(forward, angle);
		const Vector3 currentPoint = ownerPosition + direction * range;

		DebugDrawAPI::drawLine(previousPoint, currentPoint, color, 0, true);

		previousPoint = currentPoint;
	}
}

Vector3 SkeletonAttackDebugDraw::rotateAroundY(const Vector3& vector, float radians) const
{
	const float cosAngle = std::cos(radians);
	const float sinAngle = std::sin(radians);

	return Vector3(
		vector.x * cosAngle + vector.z * sinAngle,
		vector.y,
		-vector.x * sinAngle + vector.z * cosAngle
	);
}

IMPLEMENT_SCRIPT(SkeletonAttackDebugDraw)