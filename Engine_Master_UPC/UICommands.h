#pragma once
#include <string>
#include "UIRect.h"
#include "UIFill.h"
#include "CanvasRenderMode.h"
#include "SimpleMath.h"
#include "ModuleFont.h"

class Texture;

using Matrix = DirectX::SimpleMath::Matrix;

enum UITextEffectFlags
{
    UITextEffect_None = 0,
    UITextEffect_Outline = 1 << 0,
    UITextEffect_Shadow = 1 << 1,
    UITextEffect_Glow = 1 << 2,
    UITextEffect_Wave = 1 << 3
};

struct UITextCommand
{
    std::wstring text;
    float x = 0.0f;
    float y = 0.0f;
    DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float scale = 1.0f;
    int fontId = INVALID_FONT_ID;

    uint32_t effectFlags = UITextEffect_None;

    DirectX::XMFLOAT4 outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 shadowColor = { 0.0f, 0.0f, 0.0f, 0.6f };
    DirectX::XMFLOAT4 glowColor = { 1.0f, 1.0f, 1.0f, 0.5f };

    float outlineSize = 1.0f;
    float shadowOffsetX = 2.0f;
    float shadowOffsetY = 2.0f;
    float glowSize = 3.0f;
    float waveAmplitude = 2.0f;
    float waveFrequency = 0.05f;
    float waveSpeed = 4.0f;
};

struct UIImageCommand
{
    Texture* texture = nullptr;
    Rect2D  rect;

    float alpha = 1.0f;

    Vector2 fillAmount = { 0.0f, 1.0f };
    FillMethod fillMethod = FillMethod::Horizontal;
    FillOrigin fillOrigin = FillOrigin::HorizontalLeft;
    CanvasRenderMode renderMode = CanvasRenderMode::SCREEN_SPACE;

    Matrix world = Matrix::Identity;
    bool zTest = false;

    Vector2 sheetOffset = { 0.0f, 0.0f };
    Vector2 uvScale = { 1.0f, 1.0f };
};