#pragma once
#include "IDebugDrawer.h"

class DebugEditorOverlay : public IDebugDrawer
{
public:
    void draw(const RenderContext& ctx) override;
private:
    void drawBoundingBox(const Engine::BoundingBox& bbox, const ddVec3& color);
};