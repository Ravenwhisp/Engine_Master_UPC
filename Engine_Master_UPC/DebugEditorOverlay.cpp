#include "Globals.h"
#include "DebugEditorOverlay.h"

#include "Application.h"
#include "Settings.h"
#include "ModuleScene.h"
#include "ModuleScene.h"
#include "MeshRenderer.h"
#include "debug_draw.hpp"

#include "RenderContext.h"

void DebugEditorOverlay::draw(const RenderContext& ctx)
{
    const Settings* s = app->getSettings();

    if (s->sceneEditor.showGrid)
    {
        dd::xzSquareGrid(-10.0f, 10.f, 0.0f, 1.0f, dd::colors::LightGray);
    }

    if (s->sceneEditor.showAxis)
    {
        dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);

    }

    if (s->sceneEditor.showModelBoundingBoxes)
    {
        for (const MeshRenderer* renderer : app->getModuleScene()->getMeshRenderers())
        {
            drawBoundingBox(renderer->getBoundingBox(), dd::colors::Yellow);
        }

    }
}

void DebugEditorOverlay::drawBoundingBox(const Engine::BoundingBox& bbox, const ddVec3& color)
{
    const Vector3* c = bbox.getPoints();
    ddVec3 pts[8];
    for (int i = 0; i < 8; ++i) 
    { 
        pts[i][0] = c[i].x; 
        pts[i][1] = c[i].y; 
        pts[i][2] = c[i].z; 
    }
    dd::box(pts, color, 0, false);
}
