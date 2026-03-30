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

    m_playerBindings[0] = { DeviceType::Keyboard, 0 };
    m_playerBindings[1] = { DeviceType::Gamepad, 0 };

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

    m_prevGamepadButtons = m_currentGamepadButtons;
    m_prevLeftTrigger = m_currentLeftTrigger;
    m_prevRightTrigger = m_currentRightTrigger;

    for (int player = 0; player < MAX_GAMEPADS; ++player)
    {
        if (!isGamePadConnected(player))
        {
            for (int button = 0; button < GAMEPAD_BUTTON_COUNT; ++button)
            {
                m_currentGamepadButtons[player][button] = false;
            }

            m_currentLeftTrigger[player] = 0.0f;
            m_currentRightTrigger[player] = 0.0f;
            continue;
        }

        for (int button = 0; button < GAMEPAD_BUTTON_COUNT; ++button)
        {
            m_currentGamepadButtons[player][button] = SDL_GetGamepadButton(m_sdlGamepads[player], static_cast<SDL_GamepadButton>(button));
        }

        m_currentLeftTrigger[player] = std::max(0.0f, static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_LEFT_TRIGGER)) / 32767.0f);

        m_currentRightTrigger[player] = std::max(0.0f, static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)) / 32767.0f);
    }
}

void ModuleInput::setPlayerBinding(int player, DeviceType deviceType, int deviceIndex)
{
    if (player < 0 || player >= MAX_LOCAL_PLAYERS)
    {
        return;
    }

    if (deviceType == DeviceType::Keyboard)
    {
        deviceIndex = 0;
    }

    if (deviceType == DeviceType::Gamepad)
    {
        if (deviceIndex < 0 || deviceIndex >= MAX_GAMEPADS)
        {
            return;
        }
    }

    m_playerBindings[player].deviceType = deviceType;
    m_playerBindings[player].deviceIndex = deviceIndex;
}

PlayerBinding ModuleInput::getPlayerBinding(int player) const
{
    if (player < 0 || player >= MAX_LOCAL_PLAYERS)
    {
        return {};
    }

    return m_playerBindings[player];
}

bool ModuleInput::isKeyDown(Keyboard::Keys key)
{
    return m_keyboard->GetState().IsKeyDown(key);
}

#pragma region Mouse

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

#pragma endregion


#pragma region Gamepad

bool ModuleInput::isGamePadConnected(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    return m_sdlGamepads[player] != nullptr;
}

bool ModuleInput::isGamePadButtonDown(int player, SDL_GamepadButton button) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    if (button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT)
    {
        return false;
    }

    return m_currentGamepadButtons[player][button];
}

bool ModuleInput::isGamePadButtonJustPressedInternal(int player, SDL_GamepadButton button) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    if (button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT)
    {
        return false;
    }

    return m_currentGamepadButtons[player][button] && !m_prevGamepadButtons[player][button];
}

bool ModuleInput::isGamePadButtonReleasedInternal(int player, SDL_GamepadButton button) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    if (button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT)
    {
        return false;
    }

    return !m_currentGamepadButtons[player][button] && m_prevGamepadButtons[player][button];
}

Vector2 ModuleInput::getLeftStick(int player) const
{
    if (!isGamePadConnected(player))
    {
        return Vector2(0.0f, 0.0f);
    }

    const float x = static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_LEFTX)) / 32767.0f;
    const float y = static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_LEFTY)) / 32767.0f;

    Vector2 stick(x, y);

    const float deadzone = 0.15f;
    if (stick.LengthSquared() < deadzone * deadzone)
    {
        return Vector2(0.0f, 0.0f);
    }

    return stick;
}

Vector2 ModuleInput::getRightStick(int player) const
{
    if (!isGamePadConnected(player))
    {
        return Vector2(0.0f, 0.0f);
    }

    const float x = static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_RIGHTX)) / 32767.0f;
    const float y = static_cast<float>(SDL_GetGamepadAxis(m_sdlGamepads[player], SDL_GAMEPAD_AXIS_RIGHTY)) / 32767.0f;

    Vector2 stick(x, y);

    const float deadzone = 0.15f;
    if (stick.LengthSquared() < deadzone * deadzone)
    {
        return Vector2(0.0f, 0.0f);
    }

    return stick;
}

bool ModuleInput::isGamePadLeftStickPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_LEFT_STICK);
}

bool ModuleInput::isGamePadRightStickPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
}

bool ModuleInput::isGamePadLeftStickJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_LEFT_STICK);
}

bool ModuleInput::isGamePadRightStickJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
}

bool ModuleInput::isGamePadLeftStickReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_LEFT_STICK);
}

bool ModuleInput::isGamePadRightStickReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
}

bool ModuleInput::isGamePadAPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_SOUTH);
}

bool ModuleInput::isGamePadBPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_EAST);
}

bool ModuleInput::isGamePadXPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_WEST);
}

bool ModuleInput::isGamePadYPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_NORTH);
}

bool ModuleInput::isGamePadAJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_SOUTH);
}

bool ModuleInput::isGamePadBJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_EAST);
}

bool ModuleInput::isGamePadXJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_WEST);
}

bool ModuleInput::isGamePadYJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_NORTH);
}

bool ModuleInput::isGamePadAReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_SOUTH);
}

bool ModuleInput::isGamePadBReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_EAST);
}

bool ModuleInput::isGamePadXReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_WEST);
}

bool ModuleInput::isGamePadYReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_NORTH);
}

bool ModuleInput::isGamePadLeftShoulderPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
}

bool ModuleInput::isGamePadRightShoulderPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
}

bool ModuleInput::isGamePadLeftShoulderJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
}

bool ModuleInput::isGamePadRightShoulderJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
}

bool ModuleInput::isGamePadLeftShoulderReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
}

bool ModuleInput::isGamePadRightShoulderReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
}

float ModuleInput::getLeftTrigger(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return 0.0f;
    }

    return m_currentLeftTrigger[player];
}

float ModuleInput::getRightTrigger(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return 0.0f;
    }

    return m_currentRightTrigger[player];
}

bool ModuleInput::isGamePadLeftTriggerPressed(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    return m_currentLeftTrigger[player] > TRIGGER_PRESS_THRESHOLD;
}

bool ModuleInput::isGamePadRightTriggerPressed(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    return m_currentRightTrigger[player] > TRIGGER_PRESS_THRESHOLD;
}

bool ModuleInput::isGamePadLeftTriggerJustPressed(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    return m_currentLeftTrigger[player] > TRIGGER_PRESS_THRESHOLD && m_prevLeftTrigger[player] <= TRIGGER_PRESS_THRESHOLD;
}

bool ModuleInput::isGamePadRightTriggerJustPressed(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    return m_currentRightTrigger[player] > TRIGGER_PRESS_THRESHOLD && m_prevRightTrigger[player] <= TRIGGER_PRESS_THRESHOLD;
}

bool ModuleInput::isGamePadLeftTriggerReleased(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    return m_currentLeftTrigger[player] <= TRIGGER_PRESS_THRESHOLD && m_prevLeftTrigger[player] > TRIGGER_PRESS_THRESHOLD;
}

bool ModuleInput::isGamePadRightTriggerReleased(int player) const
{
    if (player < 0 || player >= MAX_GAMEPADS)
    {
        return false;
    }

    return m_currentRightTrigger[player] <= TRIGGER_PRESS_THRESHOLD && m_prevRightTrigger[player] > TRIGGER_PRESS_THRESHOLD;
}

bool ModuleInput::isGamePadDPadUpPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_DPAD_UP);
}

bool ModuleInput::isGamePadDPadDownPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
}

bool ModuleInput::isGamePadDPadLeftPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
}

bool ModuleInput::isGamePadDPadRightPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
}

bool ModuleInput::isGamePadDPadUpJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_DPAD_UP);
}

bool ModuleInput::isGamePadDPadDownJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
}

bool ModuleInput::isGamePadDPadLeftJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
}

bool ModuleInput::isGamePadDPadRightJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
}

bool ModuleInput::isGamePadDPadUpReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_DPAD_UP);
}

bool ModuleInput::isGamePadDPadDownReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
}

bool ModuleInput::isGamePadDPadLeftReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
}

bool ModuleInput::isGamePadDPadRightReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
}

bool ModuleInput::isGamePadStartPressed(int player) const
{
    return isGamePadButtonDown(player, SDL_GAMEPAD_BUTTON_START);
}

bool ModuleInput::isGamePadStartJustPressed(int player) const
{
    return isGamePadButtonJustPressedInternal(player, SDL_GAMEPAD_BUTTON_START);
}

bool ModuleInput::isGamePadStartReleased(int player) const
{
    return isGamePadButtonReleasedInternal(player, SDL_GAMEPAD_BUTTON_START);
}

#pragma endregion