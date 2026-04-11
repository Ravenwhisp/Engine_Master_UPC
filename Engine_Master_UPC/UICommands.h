#pragma once
#include <string>
#include "UIRect.h"
#include "UIFill.h"
#include "CanvasRenderMode.h"
#include "SimpleMath.h"

class Texture;

using Matrix = DirectX::SimpleMath::Matrix;

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
    FillMethod fillMethod = FillMethod::Horizontal;
    FillOrigin fillOrigin = FillOrigin::HorizontalLeft;

    CanvasRenderMode renderMode = CanvasRenderMode::SCREEN_SPACE;
    Matrix world = Matrix::Identity;
};