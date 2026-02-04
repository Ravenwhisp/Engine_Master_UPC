#pragma once

#include "Module.h"
#include "Keyboard.h"

namespace DirectX { class Keyboard; class Mouse; class GamePad;  }

class InputModule : public Module
{
public:

    InputModule(HWND hWnd);
    void update() override;
    bool IsKeyDown(Keyboard::Keys key);
    bool IsLeftMouseDown();
    bool IsRightMouseDown();
    void GetMouseDelta(float& deltaX, float& deltaY);
    void GetMouseWheel(float& delta);
private:
    std::unique_ptr<Keyboard> keyboard;
    std::unique_ptr<Mouse> mouse;
    std::unique_ptr<GamePad> gamePad;

    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float wheelDelta = 0.0f;
    bool firstMove = true;
};
