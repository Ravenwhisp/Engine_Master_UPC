#include "Globals.h"
#include "Application.h"
#include "InputModule.h"

#include "Keyboard.h"
#include "Mouse.h"
#include "GamePad.h"

InputModule::InputModule(HWND hWnd)
{
    keyboard = std::make_unique<Keyboard>();
    mouse = std::make_unique<Mouse>();
    gamePad = std::make_unique<GamePad>();

    mouse->SetWindow(hWnd);
}

void InputModule::update()
{
    if (!IsRightMouseDown() && !IsLeftMouseDown()) {
        firstMove = true;
    }
}

bool InputModule::IsKeyDown(Keyboard::Keys key)
{
    return keyboard->GetState().IsKeyDown(key);
}

bool InputModule::IsLeftMouseDown()
{
    return mouse->GetState().leftButton == Mouse::ButtonStateTracker::HELD;
}

bool InputModule::IsRightMouseDown()
{
    return mouse->GetState().rightButton == Mouse::ButtonStateTracker::HELD;
}

void InputModule::GetMouseDelta(float& deltaX, float& deltaY)
{
    auto state = mouse->GetState();

    if (!state.rightButton && !state.leftButton)
        firstMove = true;

    float currentX = (float)state.x;
    float currentY = (float)state.y;

    if (firstMove)
    {
        // Prevent huge initial jump
        mouseDeltaX = currentX;
        mouseDeltaY = currentY;
        deltaX = 0.0f;
        deltaY = 0.0f;
        firstMove = false;
        return;
    }

    deltaX = currentX - mouseDeltaX;
    deltaY = currentY - mouseDeltaY;

    mouseDeltaX = currentX;
    mouseDeltaY = currentY;
}

void InputModule::GetMouseWheel(float& delta)
{
    float currentValue = mouse->GetState().scrollWheelValue;
    delta = currentValue - wheelDelta;
    wheelDelta = currentValue;
}

