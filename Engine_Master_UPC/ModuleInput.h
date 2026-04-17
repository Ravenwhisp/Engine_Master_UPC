#pragma once

#include "Module.h"
#include "Keyboard.h"
#include "Mouse.h"

#include "DeviceType.h"
#include "PlayerBinding.h"

#include <array>
#include <SDL3/SDL.h>

class ModuleInput : public Module
{
public:
    ModuleInput(HWND hWnd);
    ~ModuleInput() override;

    void update() override;

    void setWindowSize(int width, int height)
    {
        m_windowWidth = width;
        m_windowHeight = height;
    }

    void setPlayerBinding(int player, DeviceType deviceType, int deviceIndex = 0);
    PlayerBinding getPlayerBinding(int player) const;

#pragma region Keyboard
    bool isKeyDown(Keyboard::Keys key);
    bool isKeyJustPressed(Keyboard::Keys key) const;
    bool isKeyReleased(Keyboard::Keys key) const;
#pragma endregion

#pragma region Mouse
    bool isLeftMouseDown();
    bool isRightMouseDown();

    bool isMouseButtonPressed(Mouse::ButtonStateTracker::ButtonState btn);
    bool isLeftMousePressed()   const;
    bool isRightMousePressed()  const;
    bool isMiddleMousePressed() const;


    bool isLeftMouseReleased()  const;
    bool isRightMouseReleased() const;
    bool isMiddleMouseReleased()const;

    // Returns true while the button is held down
    bool isLeftMouseHeld()      const;
    bool isRightMouseHeld()     const;
    bool isMiddleMouseHeld()    const;

    Vector2 getMousePosition()   const;

    // ---- Mouse delta & wheel --------------------------------------------
    void getMouseDelta(float& deltaX, float& deltaY);
    void getMouseWheel(float& delta);
#pragma endregion

#pragma region Gamepad
    bool isGamePadConnected(int player = 0) const;

    // Joysticks
    Vector2 getLeftStick(int player = 0) const;
    Vector2 getRightStick(int player = 0) const;

    bool isGamePadLeftStickPressed(int player = 0) const;
    bool isGamePadRightStickPressed(int player = 0) const;

    bool isGamePadLeftStickJustPressed(int player = 0) const;
    bool isGamePadRightStickJustPressed(int player = 0) const;

    bool isGamePadLeftStickReleased(int player = 0) const;
    bool isGamePadRightStickReleased(int player = 0) const;

    // Action buttons
    bool isGamePadAPressed(int player = 0) const;
    bool isGamePadBPressed(int player = 0) const;
    bool isGamePadXPressed(int player = 0) const;
    bool isGamePadYPressed(int player = 0) const;

    bool isGamePadAJustPressed(int player = 0) const;
    bool isGamePadBJustPressed(int player = 0) const;
    bool isGamePadXJustPressed(int player = 0) const;
    bool isGamePadYJustPressed(int player = 0) const;

    bool isGamePadAReleased(int player = 0) const;
    bool isGamePadBReleased(int player = 0) const;
    bool isGamePadXReleased(int player = 0) const;
    bool isGamePadYReleased(int player = 0) const;

    // Shoulder buttons
    bool isGamePadLeftShoulderPressed(int player = 0) const;
    bool isGamePadRightShoulderPressed(int player = 0) const;

    bool isGamePadLeftShoulderJustPressed(int player = 0) const;
    bool isGamePadRightShoulderJustPressed(int player = 0) const;

    bool isGamePadLeftShoulderReleased(int player = 0) const;
    bool isGamePadRightShoulderReleased(int player = 0) const;

    // Trigger buttons
    float getLeftTrigger(int player = 0) const;
    float getRightTrigger(int player = 0) const;

    bool isGamePadLeftTriggerPressed(int player = 0) const;
    bool isGamePadRightTriggerPressed(int player = 0) const;

    bool isGamePadLeftTriggerJustPressed(int player = 0) const;
    bool isGamePadRightTriggerJustPressed(int player = 0) const;

    bool isGamePadLeftTriggerReleased(int player = 0) const;
    bool isGamePadRightTriggerReleased(int player = 0) const;

    // DPad buttons
    bool isGamePadDPadUpPressed(int player = 0) const;
    bool isGamePadDPadDownPressed(int player = 0) const;
    bool isGamePadDPadLeftPressed(int player = 0) const;
    bool isGamePadDPadRightPressed(int player = 0) const;

    bool isGamePadDPadUpJustPressed(int player = 0) const;
    bool isGamePadDPadDownJustPressed(int player = 0) const;
    bool isGamePadDPadLeftJustPressed(int player = 0) const;
    bool isGamePadDPadRightJustPressed(int player = 0) const;

    bool isGamePadDPadUpReleased(int player = 0) const;
    bool isGamePadDPadDownReleased(int player = 0) const;
    bool isGamePadDPadLeftReleased(int player = 0) const;
    bool isGamePadDPadRightReleased(int player = 0) const;

    // Start Button
    bool isGamePadStartPressed(int player = 0) const;
    bool isGamePadStartJustPressed(int player = 0) const;
    bool isGamePadStartReleased(int player = 0) const;
#pragma endregion

private:
    bool isGamePadButtonDown(int player, SDL_GamepadButton button) const;
    bool isGamePadButtonJustPressedInternal(int player, SDL_GamepadButton button) const;
    bool isGamePadButtonReleasedInternal(int player, SDL_GamepadButton button) const;

private:
    // Core input devices
    std::unique_ptr<Keyboard> m_keyboard;
    std::unique_ptr<Mouse> m_mouse;
    Keyboard::KeyboardStateTracker m_keyboardTracker;
    Mouse::ButtonStateTracker m_mouseTracker;

    // Mouse state
    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
    float m_wheelDelta = 0.0f;
    bool m_firstMove = true;

    // Window state
    int m_windowWidth = 1920;
    int m_windowHeight = 1080;

    // Player bindings
    static constexpr int MAX_LOCAL_PLAYERS = 2;
    std::array<PlayerBinding, MAX_LOCAL_PLAYERS> m_playerBindings{};

    // SDL gamepad handles
    bool m_sdlInitialized = false;
    static constexpr int MAX_GAMEPADS = 2;
    std::array<SDL_Gamepad*, MAX_GAMEPADS> m_sdlGamepads{};

    // Gamepad button snapshots
    static constexpr int GAMEPAD_BUTTON_COUNT = SDL_GAMEPAD_BUTTON_COUNT;
    std::array<std::array<bool, GAMEPAD_BUTTON_COUNT>, MAX_GAMEPADS> m_currentGamepadButtons{};
    std::array<std::array<bool, GAMEPAD_BUTTON_COUNT>, MAX_GAMEPADS> m_prevGamepadButtons{};

    // Gamepad trigger snapshots
    static constexpr float TRIGGER_PRESS_THRESHOLD = 0.5f;
    std::array<float, MAX_GAMEPADS> m_currentLeftTrigger{};
    std::array<float, MAX_GAMEPADS> m_prevLeftTrigger{};
    std::array<float, MAX_GAMEPADS> m_currentRightTrigger{};
    std::array<float, MAX_GAMEPADS> m_prevRightTrigger{};
};