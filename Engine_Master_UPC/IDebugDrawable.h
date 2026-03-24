#pragma once

class IDebugDrawable {
public:
    virtual ~IDebugDrawable() = default;
    virtual void debugDraw() = 0;
};