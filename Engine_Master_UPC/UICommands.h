#pragma once
#include <string>
#include "UIRect.h"

class Texture;

struct UITextCommand
{
    std::wstring text;
    float x = 0.0f;
    float y = 0.0f;
    DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float scale = 1.0f;
};

struct UIImageCommand
{
    Texture* texture = nullptr;
    Rect2D  rect;
    
    float fillAmount = 1.0f;
    int fillMethod = 0; 
    bool clockwise = true;
};