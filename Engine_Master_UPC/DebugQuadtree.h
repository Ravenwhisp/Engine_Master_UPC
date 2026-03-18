#pragma once
#include "IDebugDrawer.h"

class DebugQuadtree : public IDebugDrawer
{
public:
    void draw(const RenderContext& ctx) override;
};