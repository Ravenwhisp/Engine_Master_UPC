#include "pch.h"
#include "FrustumTestTrigger.h"
#include "BreakableObject.h"

IMPLEMENT_SCRIPT_FIELDS(FrustumTestTrigger,
    SERIALIZED_INT(m_playerIndex, "Player Index")
)

FrustumTestTrigger::FrustumTestTrigger(GameObject* owner)
    : Script(owner)
{
}

void FrustumTestTrigger::Start()
{
    m_breakable = GameObjectAPI::findScript<BreakableObject>(getOwner());

    if (!m_breakable)
    {
        Debug::warn("FrustumTestTrigger on '%s' could not find a BreakableObject on the same GameObject.", GameObjectAPI::getName(getOwner()));
    }

    // Bind this player slot to keyboard/mouse so the mouse-click alias below works.
    Input::setPlayerKeyboard(m_playerIndex);
}

void FrustumTestTrigger::Update()
{
    const Vector3 white = { 1.0f, 1.0f, 1.0f };
    const Vector3 green = { 0.0f, 1.0f, 0.0f };
    const Vector3 red = { 1.0f, 0.0f, 0.0f };

    if (!m_breakable)
    {
        DebugDrawAPI::drawScreenText("FrustumTestTrigger: no BreakableObject found on this GameObject.", { 20.0f, 20.0f, 0.0f }, red, 1.0f, 0);
        return;
    }

    // SPACE (edge-detected) or a left mouse click break the object.
    const bool spaceDown = Input::isKeyDown(KeyCode::Space);
    const bool spaceJustPressed = spaceDown && !m_prevSpaceDown;
    m_prevSpaceDown = spaceDown;

    const bool mouseJustClicked = Input::isRightTriggerJustPressed(m_playerIndex); // aliases to left mouse button when keyboard-bound

    if ((spaceJustPressed || mouseJustClicked) && !m_breakable->isBroken())
    {
        m_breakable->onBreak();
    }

    DebugDrawAPI::drawScreenText("Press SPACE or LEFT-CLICK to break the test crate", { 20.0f, 20.0f, 0.0f }, white, 1.0f, 0);

    if (m_breakable->isBroken())
    {
        DebugDrawAPI::drawScreenText("Status: BROKEN (check if the broken model is visible with Frustum Culling ON)", { 20.0f, 40.0f, 0.0f }, red, 1.0f, 0);
    }
    else
    {
        DebugDrawAPI::drawScreenText("Status: NORMAL", { 20.0f, 40.0f, 0.0f }, green, 1.0f, 0);
    }
}

IMPLEMENT_SCRIPT(FrustumTestTrigger)
