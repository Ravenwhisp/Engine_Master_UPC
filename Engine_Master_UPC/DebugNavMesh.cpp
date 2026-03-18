#include "Globals.h"
#include "DebugNavMesh.h"

#include "Application.h"
#include "ModuleNavigation.h"
#include "debug_draw.hpp"

void DebugNavMesh::draw(const RenderContext& ctx)
{
    ModuleNavigation* nav = app->getModuleNavigation();
    if (!nav) return;

    if (nav->getDrawNavMesh() && nav->getNavMesh())
    {
        for (const auto& l : nav->getNavMeshDebugLines())
        {
            dd::line(ddConvert(l.a), ddConvert(l.b), dd::colors::Green);

        }
    }

    if (nav->hasDebugPath())
    {
        const auto& pts = nav->getDebugPathPoints();
        for (size_t i = 1; i < pts.size(); ++i)
        {
            dd::line(ddConvert(pts[i - 1]), ddConvert(pts[i]), dd::colors::Yellow);
        }


        dd::line(ddConvert(pts.front()), ddConvert(pts.front() + Vector3(0, 0.25f, 0)), dd::colors::Yellow);
        dd::line(ddConvert(pts.back()), ddConvert(pts.back() + Vector3(0, 0.25f, 0)), dd::colors::Yellow);
    }
}