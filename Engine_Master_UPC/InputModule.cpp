#include "Globals.h"
#include "Application.h"
#include "InputModule.h"

#include "Keyboard.h"
#include "Mouse.h"
#include "GamePad.h"

InputModule::InputModule(HWND hWnd)
{
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_gamePad = std::make_unique<GamePad>();

    m_mouse->SetWindow(hWnd);
}

void InputModule::update()
{
    if (!isRightMouseDown() && !isLeftMouseDown()) 
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
    return m_mouse->GetState().leftButton == Mouse::ButtonStateTracker::HELD;
}

bool InputModule::isRightMouseDown()
{
    return m_mouse->GetState().rightButton == Mouse::ButtonStateTracker::HELD;
}

void InputModule::getMouseDelta(float& deltaX, float& deltaY)
{
    auto state = m_mouse->GetState();

    if (!state.rightButton && !state.leftButton) 
    {
        m_firstMove = true;
    }

    float currentX = (float)state.x;
    float currentY = (float)state.y;

    if (m_firstMove)
    {
        // Prevent huge initial jump
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
    float currentValue = m_mouse->GetState().scrollWheelValue;
    delta = currentValue - m_wheelDelta;
    m_wheelDelta = currentValue;
}

