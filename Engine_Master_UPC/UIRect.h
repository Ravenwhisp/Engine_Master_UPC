#pragma once

struct Rect2D
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool contains(const Vector2& p) const
    {
        return p.x >= x
            && p.x <= x + w
            && p.y >= y
            && p.y <= y + h;
    }

};