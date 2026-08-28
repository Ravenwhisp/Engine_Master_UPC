#include "pch.h"
#include "MouseAimTest.h"

#include <cstdio>

IMPLEMENT_SCRIPT_FIELDS(MouseAimTest,
    SERIALIZED_INT(m_playerIndex, "Player Index"),
    SERIALIZED_FLOAT(m_arrowLength, "Arrow Length", 0.1f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_arrowSize, "Arrow Size", 0.01f, 2.0f, 0.01f)
)

MouseAimTest::MouseAimTest(GameObject* owner)
    : Script(owner)
{
}

void MouseAimTest::Update()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    const Vector3 origin = TransformAPI::getGlobalPosition(ownerTransform);

    const Vector3 white = { 1.0f, 1.0f, 1.0f };
    const Vector3 red = { 1.0f, 0.0f, 0.0f };
    const Vector3 yellow = { 1.0f, 1.0f, 0.0f };

    DebugDrawAPI::drawPoint(origin, white, 6.0f, 0, true);

    const Vector3 aimDirection = Input::getAimDirection(origin, m_playerIndex);

    if (aimDirection.LengthSquared() <= 0.0001f)
    {
        DebugDrawAPI::drawScreenText("No aim input (deadzone / mouse outside Game viewport)", { 20.0f, 20.0f, 0.0f }, red, 1.0f, 0);
        return;
    }

    char dirText[128];
    snprintf(dirText, sizeof(dirText), "Aim Dir (P%d): (%.2f, %.2f, %.2f)", m_playerIndex, aimDirection.x, aimDirection.y, aimDirection.z);
    DebugDrawAPI::drawScreenText(dirText, { 20.0f, 20.0f, 0.0f }, yellow, 1.0f, 0);

    const Vector3 aimEnd = origin + aimDirection * m_arrowLength;
    DebugDrawAPI::drawArrow(origin, aimEnd, yellow, m_arrowSize, 0, true);
    DebugDrawAPI::drawSphere(aimEnd, yellow, m_arrowSize * 0.5f, 0, true);
}

IMPLEMENT_SCRIPT(MouseAimTest)
