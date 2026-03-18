#pragma once
#include "IDebugDrawer.h"

class DebugNavMesh : public IDebugDrawer
{
public:
    void draw(const RenderContext& ctx) override;
};