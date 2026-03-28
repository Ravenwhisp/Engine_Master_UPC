#include "Globals.h"
#include "Application.h"
#include "ModuleInput.h"

#include "Keyboard.h"
#include "Mouse.h"
#include "GamePad.h"

#include "ModuleEditor.h"

#include <SDL3/SDL.h>

ModuleInput::ModuleInput(HWND hWnd)
{
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();

    m_mouse->SetWindow(hWnd);

    m_sdlInitialized = SDL_Init(SDL_INIT_GAMEPAD);
    if (!m_sdlInitialized)
    {
        return;
    }

    int count = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
    if (!gamepads)
    {
        return;
    }

    for (int i = 0; i < count && i < MAX_GAMEPADS; ++i)
    {
        m_sdlGamepads[i] = SDL_OpenGamepad(gamepads[i]);
    }

    SDL_free(gamepads);
}

ModuleInput::~ModuleInput()
{
    for (SDL_Gamepad*& gamepad : m_sdlGamepads)
    {
        if (gamepad)
        {
            SDL_CloseGamepad(gamepad);
            gamepad = nullptr;
        }
    }

    if (m_sdlInitialized)
    {
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        m_sdlInitialized = false;
    }
}


void ModuleInput::update()
{
    m_mouseTracker.Update(m_mouse->GetState());

    if (m_sdlInitialized)
    {
        SDL_PumpEvents();
    }

    if (!isLeftMouseHeld() && !isRightMouseHeld())
    {
        m_firstMove = true;
    }
}


bool ModuleInput::isKeyDown(Keyboard::Keys key)
{
    return m_keyboard->GetState().IsKeyDown(key);
}


bool ModuleInput::isLeftMouseDown()
{
    return m_mouseTracker.leftButton == Mouse::ButtonStateTracker::HELD;
}

bool ModuleInput::isRightMouseDown()
{
    return m_mouseTracker.rightButton == Mouse::ButtonStateTracker::HELD;
}


bool ModuleInput::isLeftMousePressed() const
{
    return m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED;
}

bool ModuleInput::isRightMousePressed() const
{
    return m_mouseTracker.rightButton == Mouse::ButtonStateTracker::PRESSED;
}

bool ModuleInput::isMiddleMousePressed() const
{
    return m_mouseTracker.middleButton == Mouse::ButtonStateTracker::PRESSED;
}


bool ModuleInput::isLeftMouseReleased() const
{
    return m_mouseTracker.leftButton == Mouse::ButtonStateTracker::RELEASED;
}

bool ModuleInput::isRightMouseReleased() const
{
    return m_mouseTracker.rightButton == Mouse::ButtonStateTracker::RELEASED;
}

bool ModuleInput::isMiddleMouseReleased() const
{
    return m_mouseTracker.middleButton == Mouse::ButtonStateTracker::RELEASED;
}


bool ModuleInput::isLeftMouseHeld() const
{
    return m_mouseTracker.leftButton == Mouse::ButtonStateTracker::HELD
        || m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED;
}

bool ModuleInput::isRightMouseHeld() const
{
    return m_mouseTracker.rightButton == Mouse::ButtonStateTracker::HELD
        || m_mouseTracker.rightButton == Mouse::ButtonStateTracker::PRESSED;
}

bool ModuleInput::isMiddleMouseHeld() const
{
    return m_mouseTracker.middleButton == Mouse::ButtonStateTracker::HELD
        || m_mouseTracker.middleButton == Mouse::ButtonStateTracker::PRESSED;
}


Vector2 ModuleInput::getMousePosition() const
{
    const Mouse::State state = m_mouse->GetState();
    return { static_cast<float>(state.x), static_cast<float>(state.y) };
}

void ModuleInput::getMouseDelta(float& deltaX, float& deltaY)
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


void ModuleInput::getMouseWheel(float& delta)
{
    const float currentValue = static_cast<float>(m_mouse->GetState().scrollWheelValue);
    delta = currentValue - m_wheelDelta;
    m_wheelDelta = currentValue;
}

// GamePad

bool ModuleInput::isGamePadConnected(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    return m_sdlGamepads[player] != nullptr;
}

Vector2 ModuleInput::getLeftStick(int player) const
{
    if (!isGamePadConnected(player))
    {
        return Vector2(0.0f, 0.0f);
    }

    const float x = static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_LEFTX)) / 32767.0f;
    const float y = static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_LEFTY)) / 32767.0f;

    return Vector2(x, y);
}

Vector2 ModuleInput::getRightStick(int player) const
{
    if (!isGamePadConnected(player))
    {
        return Vector2(0.0f, 0.0f);
    }

    const float x = static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_RIGHTX)) / 32767.0f;
    const float y = static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_RIGHTY)) / 32767.0f;

    return Vector2(x, y);
}

float ModuleInput::getLeftTrigger(int player) const
{
    if (!isGamePadConnected(player))
    {
        return 0.0f;
    }

    return std::max(0.0f,
        static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_LEFT_TRIGGER)) / 32767.0f);
}

float ModuleInput::getRightTrigger(int player) const
{
    if (!isGamePadConnected(player))
    {
        return 0.0f;
    }

    return std::max(0.0f,
        static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)) / 32767.0f);
}

bool ModuleInput::isGamePadAPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_SOUTH);
}

bool ModuleInput::isGamePadBPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_EAST);
}

bool ModuleInput::isGamePadXPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_WEST);
}

bool ModuleInput::isGamePadYPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_NORTH);
}

bool ModuleInput::isGamePadLeftShoulderPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
}

bool ModuleInput::isGamePadRightShoulderPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
}

bool ModuleInput::isGamePadDPadUpPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_DPAD_UP);
}

bool ModuleInput::isGamePadDPadDownPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_DPAD_DOWN);
}

bool ModuleInput::isGamePadDPadLeftPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_DPAD_LEFT);
}

bool ModuleInput::isGamePadDPadRightPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
}

bool ModuleInput::isGamePadStartPressed(int player) const
{
    return isGamePadConnected(player)
        && SDL_GetGamepadButton(m_sdlGamepads[player], SDL_GAMEPAD_BUTTON_START);
}