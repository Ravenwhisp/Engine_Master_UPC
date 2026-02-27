#pragma once
#include "Component.h"

struct Float2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct Rect2D
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

class Transform2D : public Component
{
public:
    Transform2D(UID id, GameObject* owner);

    Float2 position{ 0.0f, 0.0f }; 
    Float2 size{ 100.0f, 100.0f };
    Float2 pivot{ 0.5f, 0.5f };

    Rect2D getRect() const;

    void drawUi() override;
};