#pragma once

#include "Module.h"
#include "Keyboard.h"

namespace DirectX { class Keyboard; class Mouse; class GamePad;  }

class InputModule : public Module
{
public:

    InputModule(HWND hWnd);

    void update() override;

    bool isKeyDown(Keyboard::Keys key);
    bool isLeftMouseDown();
    bool isRightMouseDown();
    void getMouseDelta(float& deltaX, float& deltaY);
    void getMouseWheel(float& delta);
private:
    std::unique_ptr<Keyboard> m_keyboard;
    std::unique_ptr<Mouse> m_mouse;
    std::unique_ptr<GamePad> m_gamePad;

    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
    float m_wheelDelta = 0.0f;
    bool m_firstMove = true;
};
