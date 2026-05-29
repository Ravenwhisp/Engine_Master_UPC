#include "pch.h"
#include "ArthurAttackDebugDraw.h"
#include "ArthurAttackConfig.h"
#include "ArthurBossController.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(ArthurAttackDebugDraw,
    SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled"),
    SERIALIZED_BOOL(m_drawHeavySwipe, "Draw Heavy Swipe"),
    SERIALIZED_BOOL(m_drawSideSweepRight, "Draw Side Sweep Right"),
    SERIALIZED_BOOL(m_drawSideSweepLeft, "Draw Side Sweep Left"),
    SERIALIZED_BOOL(m_drawEarthHammer, "Draw Earth Hammer"),
    SERIALIZED_BOOL(m_drawChargingSlam, "Draw Charging Slam"),
    SERIALIZED_FLOAT(m_heightOffset, "Height Offset", 0.0f, 5.0f, 0.05f)
)

ArthurAttackDebugDraw::ArthurAttackDebugDraw(GameObject* owner)
    : Script(owner)
{
}

void ArthurAttackDebugDraw::Start()
{
    m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());

    if (!m_attackConfig)
    {
        Debug::error("[ArthurAttackDebugDraw] ArthurAttackConfig not found.");
    }
}

void ArthurAttackDebugDraw::drawGizmo()
{
    if (!m_debugEnabled)
    {
        return;
    }

    if (!m_attackConfig)
    {
        m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());
    }

    if (!m_attackConfig)
    {
        return;
    }

    if (m_drawHeavySwipe)
    {
        drawHeavySwipeCone();
    }

    if (m_drawSideSweepLeft)
    {
        drawSideSweepCone(1);
    }

    if (m_drawSideSweepRight)
    {
        drawSideSweepCone(-1);
    }

    if (m_drawEarthHammer)
    {
        drawEarthHammerRadius();
    }

    if (m_drawChargingSlam)
    {
        drawChargingSlamPreview();
    }
}

void ArthurAttackDebugDraw::drawHeavySwipeCone() const
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
        const Vector3 red = { 1.0f, 0.0f, 0.0f };
        DebugDrawAPI::drawCross(ownerPosition, 0.5f, 0, true);
        DebugDrawAPI::drawPoint(ownerPosition, red, 8.0f, 0, true);
        return;
    }

    forward.Normalize();

    constexpr float degreesToRadians = 3.14159265f / 180.0f;

    const float range = m_attackConfig->m_heavySwipeRange;
    const float halfAngleRadians = m_attackConfig->m_heavySwipeHalfAngleDegrees * degreesToRadians;

    const Vector3 heavySwipeColor = { 0.6f, 0.0f, 1.0f };

    Vector3 leftDirection = rotateAroundY(forward, -halfAngleRadians);
    Vector3 rightDirection = rotateAroundY(forward, halfAngleRadians);

    Vector3 leftPoint = ownerPosition + leftDirection * range;
    Vector3 rightPoint = ownerPosition + rightDirection * range;
    Vector3 forwardPoint = ownerPosition + forward * range;

    DebugDrawAPI::drawLine(ownerPosition, leftPoint, heavySwipeColor, 0, true);
    DebugDrawAPI::drawLine(ownerPosition, rightPoint, heavySwipeColor, 0, true);
    DebugDrawAPI::drawLine(ownerPosition, forwardPoint, heavySwipeColor, 0, true);

    const int arcSegments = 24;
    Vector3 previousPoint = leftPoint;

    for (int i = 1; i <= arcSegments; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(arcSegments);
        float angle = -halfAngleRadians + (halfAngleRadians * 2.0f * t);

        Vector3 direction = rotateAroundY(forward, angle);
        Vector3 currentPoint = ownerPosition + direction * range;

        DebugDrawAPI::drawLine(previousPoint, currentPoint, heavySwipeColor, 0, true);

        previousPoint = currentPoint;
    }
}

void ArthurAttackDebugDraw::drawEarthHammerRadius() const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    ownerPosition.y += m_heightOffset;

    const Vector3 earthHammerColor = { 0.6f, 0.0f, 1.0f };

    DebugDrawAPI::drawCircle(ownerPosition, Vector3(0.0f, 1.0f, 0.0f), earthHammerColor, m_attackConfig->m_earthHammerRadius, 48.0f, 0, true);
}

void ArthurAttackDebugDraw::drawSideSweepCone(int side) const
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

    constexpr float halfPi = 3.14159265f * 0.5f;
    constexpr float degreesToRadians = 3.14159265f / 180.0f;

    Vector3 sideDirection = rotateAroundY(forward, halfPi * static_cast<float>(side));
    sideDirection.Normalize();

    const float range = m_attackConfig->m_sideSweepRange;
    const float halfAngleRadians = m_attackConfig->m_sideSweepHalfAngleDegrees * degreesToRadians;

    const Vector3 sideSweepColor = { 0.6f, 0.0f, 1.0f };

    Vector3 leftDirection = rotateAroundY(sideDirection, -halfAngleRadians);
    Vector3 rightDirection = rotateAroundY(sideDirection, halfAngleRadians);

    Vector3 leftPoint = ownerPosition + leftDirection * range;
    Vector3 rightPoint = ownerPosition + rightDirection * range;
    Vector3 centerPoint = ownerPosition + sideDirection * range;

    DebugDrawAPI::drawLine(ownerPosition, leftPoint, sideSweepColor, 0, true);
    DebugDrawAPI::drawLine(ownerPosition, rightPoint, sideSweepColor, 0, true);
    DebugDrawAPI::drawLine(ownerPosition, centerPoint, sideSweepColor, 0, true);

    const int arcSegments = 24;
    Vector3 previousPoint = leftPoint;

    for (int i = 1; i <= arcSegments; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(arcSegments);
        float angle = -halfAngleRadians + (halfAngleRadians * 2.0f * t);

        Vector3 direction = rotateAroundY(sideDirection, angle);
        Vector3 currentPoint = ownerPosition + direction * range;

        DebugDrawAPI::drawLine(previousPoint, currentPoint, sideSweepColor, 0, true);

        previousPoint = currentPoint;
    }
}

void ArthurAttackDebugDraw::drawChargingSlamPreview() const
{
    if (!m_attackConfig)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 startPosition = TransformAPI::getGlobalPosition(ownerTransform);
    startPosition.y += m_heightOffset;

    // This preview uses the current focus target.
    // It is not the locked runtime position, but it is enough to visualize the intended charge.
    ArthurBossController* arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    if (!arthurController)
    {
        return;
    }

    Transform* targetTransform = arthurController->getFocusTarget();
    if (!targetTransform)
    {
        return;
    }

    Vector3 endPosition = TransformAPI::getGlobalPosition(targetTransform);
    endPosition.y = startPosition.y;

    const Vector3 purple = { 0.6f, 0.0f, 1.0f };

    DebugDrawAPI::drawLine(startPosition, endPosition, purple, 0, true);

    DebugDrawAPI::drawCircle(
        startPosition,
        Vector3(0.0f, 1.0f, 0.0f),
        purple,
        m_attackConfig->m_chargingSlamDashHitRadius,
        24.0f,
        0,
        true
    );

    DebugDrawAPI::drawCircle(
        endPosition,
        Vector3(0.0f, 1.0f, 0.0f),
        purple,
        m_attackConfig->m_chargingSlamImpactRadius,
        48.0f,
        0,
        true
    );
}

Vector3 ArthurAttackDebugDraw::rotateAroundY(const Vector3& vector, float radians) const
{
    const float cosAngle = std::cos(radians);
    const float sinAngle = std::sin(radians);

    return Vector3(vector.x * cosAngle + vector.z * sinAngle, vector.y, -vector.x * sinAngle + vector.z * cosAngle);
}

IMPLEMENT_SCRIPT(ArthurAttackDebugDraw)