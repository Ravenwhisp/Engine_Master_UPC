#include "Globals.h"
#include "Application.h"
#include "InputModule.h"

#include "Keyboard.h"
#include "Mouse.h"
#include "GamePad.h"

#include "EditorModule.h"

InputModule::InputModule(HWND hWnd)
{
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_gamePad = std::make_unique<GamePad>();

    m_mouse->SetWindow(hWnd);
}


void InputModule::update()
{
    m_mouseTracker.Update(m_mouse->GetState());

    if (!isLeftMouseHeld() && !isRightMouseHeld())
    {
        m_firstMove = true;
    }
}


bool InputModule::isKeyDown(Keyboard::Keys key)
{
    return m_keyboard->GetState().IsKeyDown(key);
}


bool InputModule::isLeftMouseDown()
{
    return m_mouseTracker.leftButton == Mouse::ButtonStateTracker::HELD;
}

bool InputModule::isRightMouseDown()
{
    return m_mouseTracker.rightButton == Mouse::ButtonStateTracker::HELD;
}


bool InputModule::isLeftMousePressed() const
{
    return m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED;
}

bool InputModule::isRightMousePressed() const
{
    return m_mouseTracker.rightButton == Mouse::ButtonStateTracker::PRESSED;
}

bool InputModule::isMiddleMousePressed() const
{
    return m_mouseTracker.middleButton == Mouse::ButtonStateTracker::PRESSED;
}


bool InputModule::isLeftMouseReleased() const
{
    return m_mouseTracker.leftButton == Mouse::ButtonStateTracker::RELEASED;
}

bool InputModule::isRightMouseReleased() const
{
    return m_mouseTracker.rightButton == Mouse::ButtonStateTracker::RELEASED;
}

bool InputModule::isMiddleMouseReleased() const
{
    return m_mouseTracker.middleButton == Mouse::ButtonStateTracker::RELEASED;
}


bool InputModule::isLeftMouseHeld() const
{
    return m_mouseTracker.leftButton == Mouse::ButtonStateTracker::HELD
        || m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED;
}

bool InputModule::isRightMouseHeld() const
{
    return m_mouseTracker.rightButton == Mouse::ButtonStateTracker::HELD
        || m_mouseTracker.rightButton == Mouse::ButtonStateTracker::PRESSED;
}

bool InputModule::isMiddleMouseHeld() const
{
    return m_mouseTracker.middleButton == Mouse::ButtonStateTracker::HELD
        || m_mouseTracker.middleButton == Mouse::ButtonStateTracker::PRESSED;
}


Vector2 InputModule::getMousePosition() const
{
    const Mouse::State state = m_mouse->GetState();
    return { static_cast<float>(state.x), static_cast<float>(state.y) };
}

void InputModule::getMouseDelta(float& deltaX, float& deltaY)
{
    const Mouse::State state = m_mouse->GetState();

    const float currentX = static_cast<float>(state.x);
    const float currentY = static_cast<float>(state.y);

    if (m_firstMove)
    {
        m_mouseDeltaX = currentX;
        m_mouseDeltaY = currentY;
        deltaX = 0.0f;
        deltaY = 0.0f;
        m_firstMove = false;
        return;
    }

    deltaX = currentX - m_mouseDeltaX;
    deltaY = currentY - m_mouseDeltaY;

    m_mouseDeltaX = currentX;
    m_mouseDeltaY = currentY;
}


void InputModule::getMouseWheel(float& delta)
{
    const float currentValue = static_cast<float>(m_mouse->GetState().scrollWheelValue);
    delta = currentValue - m_wheelDelta;
    m_wheelDelta = currentValue;
}