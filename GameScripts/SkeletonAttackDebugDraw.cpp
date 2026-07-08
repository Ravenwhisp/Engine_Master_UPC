#include "pch.h"
#include "SkeletonAttackDebugDraw.h"

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
	m_attackConfig = GameObjectAPI::findScript<SkeletonAttackConfig>(getOwner());

	if (!m_attackConfig)
	{
		Debug::warn("[SkeletonAttackDebugDraw] SkeletonAttackConfig not found.");
	}
}

void SkeletonAttackDebugDraw::drawGizmo()
{
	if (!m_debugEnabled)
	{
		return;
	}

	if (!m_attackConfig)
	{
		m_attackConfig = GameObjectAPI::findScript<SkeletonAttackConfig>(getOwner());
	}

	if (!m_attackConfig)
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

	const Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

	const Vector3 startRangeColor = Vector3(0.0f, 1.0f, 1.0f);
	const Vector3 dashStopColor = Vector3(1.0f, 1.0f, 0.0f);

	if (m_drawScimitarStartRange)
	{
		DebugDrawAPI::drawCircle(
			position,
			up,
			startRangeColor,
			m_attackConfig->m_scimitarStartRange,
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
			m_attackConfig->m_scimitarDashStopRange,
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

void SkeletonAttackDebugDraw::drawScimitarAttackCone() const
{
	drawScimitarCone(
		m_attackConfig->m_basicAttackRange,
		m_attackConfig->m_scimitarHalfAngleDegrees,
		Vector3(1.0f, 0.0f, 0.0f)
	);
}

void SkeletonAttackDebugDraw::drawScimitarStunCone() const
{
	drawScimitarCone(
		m_attackConfig->m_scimitarStunHitRange,
		m_attackConfig->m_scimitarHalfAngleDegrees,
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
		DebugDrawAPI::drawCross(ownerPosition, 0.5f, 0, true);
		DebugDrawAPI::drawPoint(ownerPosition, color, 8.0f, 0, true);
		return;
	}

	forward.Normalize();

	constexpr float degreesToRadians = 3.14159265f / 180.0f;
	const float halfAngleRadians = halfAngleDegrees * degreesToRadians;

	Vector3 leftDirection = rotateAroundY(forward, -halfAngleRadians);
	Vector3 rightDirection = rotateAroundY(forward, halfAngleRadians);

	Vector3 leftPoint = ownerPosition + leftDirection * range;
	Vector3 rightPoint = ownerPosition + rightDirection * range;
	Vector3 forwardPoint = ownerPosition + forward * range;

	DebugDrawAPI::drawLine(ownerPosition, leftPoint, color, 0, true);
	DebugDrawAPI::drawLine(ownerPosition, rightPoint, color, 0, true);
	DebugDrawAPI::drawLine(ownerPosition, forwardPoint, color, 0, true);

	const int arcSegments = 24;
	Vector3 previousPoint = leftPoint;

	for (int i = 1; i <= arcSegments; ++i)
	{
		float t = static_cast<float>(i) / static_cast<float>(arcSegments);
		float angle = -halfAngleRadians + (halfAngleRadians * 2.0 * t);

		Vector3 direction = rotateAroundY(forward, angle);
		Vector3 currentPoint = ownerPosition + direction * range;

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