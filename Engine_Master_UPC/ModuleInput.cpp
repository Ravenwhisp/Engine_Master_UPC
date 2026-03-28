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
    m_gamePad = std::make_unique<GamePad>();

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

    if (count > 0)
    {
        m_sdlGamepad = SDL_OpenGamepad(gamepads[0]);
    }

    SDL_free(gamepads);
}

ModuleInput::~ModuleInput()
{
    if (m_sdlGamepad)
    {
        SDL_CloseGamepad(m_sdlGamepad);
        m_sdlGamepad = nullptr;
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

    //Test
    SDL_PumpEvents();
    // or SDL_UpdateGamepads();

    if (m_sdlGamepad)
    {
        const Sint16 lxRaw = SDL_GetGamepadAxis(m_sdlGamepad, SDL_GAMEPAD_AXIS_LEFTX);
        const Sint16 lyRaw = SDL_GetGamepadAxis(m_sdlGamepad, SDL_GAMEPAD_AXIS_LEFTY);
        const bool south = SDL_GetGamepadButton(m_sdlGamepad, SDL_GAMEPAD_BUTTON_SOUTH);

        DEBUG_LOG("SDL Gamepad: LX=%d LY=%d SOUTH=%d", (int)lxRaw, (int)lyRaw, south ? 1 : 0);
    }
    else
    {
        DEBUG_LOG("SDL Gamepad NOT opened");
    }
    //Test

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
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected();
}

Vector2 ModuleInput::getLeftStick(int player) const
{
    const auto state = m_gamePad->GetState(player);
    if (!state.IsConnected())
    {
        return Vector2(0.0f, 0.0f);
    }

    return Vector2(state.thumbSticks.leftX, state.thumbSticks.leftY);
}

Vector2 ModuleInput::getRightStick(int player) const
{
    const auto state = m_gamePad->GetState(player);
    if (!state.IsConnected())
    {
        return Vector2(0.0f, 0.0f);
    }

    return Vector2(state.thumbSticks.rightX, state.thumbSticks.rightY);
}

float ModuleInput::getLeftTrigger(int player) const
{
    const auto state = m_gamePad->GetState(player);
    if (!state.IsConnected())
    {
        return 0.0f;
    }

    return state.triggers.left;
}

float ModuleInput::getRightTrigger(int player) const
{
    const auto state = m_gamePad->GetState(player);
    if (!state.IsConnected())
    {
        return 0.0f;
    }

    return state.triggers.right;
}

bool ModuleInput::isGamePadAPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsAPressed();
}

bool ModuleInput::isGamePadBPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsBPressed();
}

bool ModuleInput::isGamePadXPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsXPressed();
}

bool ModuleInput::isGamePadYPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsYPressed();
}

bool ModuleInput::isGamePadLeftShoulderPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsLeftShoulderPressed();
}

bool ModuleInput::isGamePadRightShoulderPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsRightShoulderPressed();
}

bool ModuleInput::isGamePadDPadUpPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsDPadUpPressed();
}

bool ModuleInput::isGamePadDPadDownPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsDPadDownPressed();
}

bool ModuleInput::isGamePadDPadLeftPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsDPadLeftPressed();
}

bool ModuleInput::isGamePadDPadRightPressed(int player) const
{
    const auto state = m_gamePad->GetState(player);
    return state.IsConnected() && state.IsDPadRightPressed();
}