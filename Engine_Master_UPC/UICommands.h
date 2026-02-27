#pragma once
#include <string>
#include "UIRect.h"

class Texture;

struct UITextCommand
{
    std::wstring text;
    float x = 0.0f;
    float y = 0.0f;
};

struct UIImageCommand
{
    Texture* texture = nullptr;
    Rect2D  rect;
};