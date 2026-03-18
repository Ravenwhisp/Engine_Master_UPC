#pragma once

struct RenderContext;

class IDebugDrawer
{
public:
    virtual void draw(const RenderContext& ctx) = 0;
    virtual ~IDebugDrawer() = default;
};