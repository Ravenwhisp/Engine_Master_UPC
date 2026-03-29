#pragma once

#include "Module.h"
#include "Keyboard.h"
#include "Mouse.h"

#include "DeviceType.h"
#include "PlayerBinding.h"
#include <array>

struct SDL_Gamepad;

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

    bool isKeyDown(Keyboard::Keys key);

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

    // GamePad
    bool isGamePadConnected(int player = 0) const;

    Vector2 getLeftStick(int player = 0) const;
    Vector2 getRightStick(int player = 0) const;

    float getLeftTrigger(int player = 0) const;
    float getRightTrigger(int player = 0) const;

    bool isGamePadAPressed(int player = 0) const;
    bool isGamePadBPressed(int player = 0) const;
    bool isGamePadXPressed(int player = 0) const;
    bool isGamePadYPressed(int player = 0) const;

    bool isGamePadLeftShoulderPressed(int player = 0) const;
    bool isGamePadRightShoulderPressed(int player = 0) const;

    bool isGamePadDPadUpPressed(int player = 0) const;
    bool isGamePadDPadDownPressed(int player = 0) const;
    bool isGamePadDPadLeftPressed(int player = 0) const;
    bool isGamePadDPadRightPressed(int player = 0) const;

    bool isGamePadStartPressed(int player = 0) const;

private:
    std::unique_ptr<Keyboard>           m_keyboard;
    std::unique_ptr<Mouse>              m_mouse;
    Mouse::ButtonStateTracker           m_mouseTracker;

    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
    float m_wheelDelta = 0.0f;
    bool  m_firstMove = true;

    int m_windowWidth = 1920;
    int m_windowHeight = 1080;
    
    static constexpr int MAX_LOCAL_PLAYERS = 2;
    std::array<PlayerBinding, MAX_LOCAL_PLAYERS> m_playerBindings{};

    bool m_sdlInitialized = false;
    static constexpr int MAX_GAMEPADS = 2;
    std::array<SDL_Gamepad*, MAX_GAMEPADS> m_sdlGamepads{};
};