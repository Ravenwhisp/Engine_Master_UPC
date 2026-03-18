#include "Globals.h"
#include "DebugQuadtree.h"

#include "Application.h"
#include "Settings.h"
#include "ModuleScene.h"
#include "Quadtree.h"
#include "BoundingBox.h"
#include "debug_draw.hpp"

void DebugQuadtree::draw(const RenderContext& ctx)
{
    if (!app->getSettings()->sceneEditor.showQuadTree)
    {
        return;
    }

    Quadtree* quadtree = app->getModuleScene()->getQuadtree();
    if (!quadtree) return;

    for (const BoundingRect& rect : quadtree->getQuadrants())
    {
        Vector3 extents(rect.width * 0.5f, 0.0f, rect.height * 0.5f);
        Vector3 center(rect.x + rect.width * 0.5f, 0.1f, rect.y + rect.height * 0.5f);

        const ddVec3& color = rect.m_debugIsCulled ? dd::colors::Red : dd::colors::Green;
        dd::box(ddConvert(center), color, extents.x * 2.f, extents.y * 2.f, extents.z * 2.f);

        center.y = 0.f;
        extents.y = 10000.f;
        Vector3 min = center - extents;
        Vector3 max = center + extents;

        Vector3 pts[8] = {
            {center.x - extents.x, center.y - extents.y, center.z - extents.z},
            {center.x - extents.x, center.y - extents.y, center.z + extents.z},
            {center.x - extents.x, center.y + extents.y, center.z - extents.z},
            {center.x - extents.x, center.y + extents.y, center.z + extents.z},
            {center.x + extents.x, center.y - extents.y, center.z - extents.z},
            {center.x + extents.x, center.y - extents.y, center.z + extents.z},
            {center.x + extents.x, center.y + extents.y, center.z - extents.z},
            {center.x + extents.x, center.y + extents.y, center.z + extents.z},
        };
        Engine::BoundingBox(min, max, pts).render();
    }
}
