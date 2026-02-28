#pragma once

#include "Module.h"
#include "Keyboard.h"
#include "Mouse.h"
#include <GamePad.h>

class InputModule : public Module
{
public:
    InputModule(HWND hWnd);

    void update() override;


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

private:
    std::unique_ptr<Keyboard>           m_keyboard;
    std::unique_ptr<Mouse>              m_mouse;
    std::unique_ptr<GamePad>            m_gamePad;
    Mouse::ButtonStateTracker           m_mouseTracker;

    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
    float m_wheelDelta = 0.0f;
    bool  m_firstMove = true;
};