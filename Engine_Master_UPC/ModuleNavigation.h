#pragma once
#include "Module.h"
#include "IDebugDrawable.h"
#include "NavMeshTypes.h"
#include "NavMeshBuilder.h"

#include <vector>
#include <string>
#include <DetourNavMesh.h>
#include "NavMeshResource.h"

class dtNavMesh;
class dtNavMeshQuery;
class Scene;

class ModuleNavigation : public Module, public IDebugDrawable
{
public:
    bool init() override;
    bool cleanUp() override;

    dtNavMesh* getNavMesh() const { return m_navMesh; }
    dtNavMeshQuery* getNavQuery() const { return m_navQuery; }
    const std::string& getLoadedScene() const { return m_loadedScene; }

    bool loadNavMeshForScene(const char* sceneName);
    bool unloadNavMesh();
    bool buildNavMeshForCurrentScene();
    bool hasNavMesh() const { return m_navMesh != nullptr && m_navQuery != nullptr; }

    NavMeshBuildSettings& getSettings() { return m_settings; }

    struct NavDebugLine
    {
        Vector3 a;
        Vector3 b;
        const float* color = nullptr;
    };

    const std::vector<NavDebugLine>& getNavMeshDebugLines() const { return m_navDebugLines; }
    void rebuildNavMeshDebugLines();

    void setDrawNavMesh(bool v) { m_drawNavMesh = v; }
    bool getDrawNavMesh() const { return m_drawNavMesh; }

    void setPathStart(const Vector3& p, NavAgentProfile profile);
    void setPathEnd(const Vector3& p, NavAgentProfile profile);
    const std::vector<Vector3>& getDebugPathPoints() const { return m_debugPathPoints; }
    bool hasDebugPath() const { return m_debugPathPoints.size() >= 2; }
    bool findStraightPath(const Vector3& start, const Vector3& end, std::vector<Vector3>& outPath, const Vector3& extents, NavAgentProfile profile) const;
    bool isSegmentBlockedByRuntimeBlockers(const Vector3& from, const Vector3& to) const;
    bool isPointBlockedByRuntimeBlockers(const Vector3& point) const;

    void debugDraw() override;

    IDebugDrawable* getAsDebugDrawable() { return static_cast<IDebugDrawable*>(this); }

    unsigned short getIncludeFlagsForProfile(NavAgentProfile profile) const;

private:
    NavMeshBuildSettings m_settings;
    dtNavMesh* m_navMesh = nullptr;
    dtNavMeshQuery* m_navQuery = nullptr;
    std::vector<dtTileRef> m_tileRefs;

    std::string m_loadedScene;
    bool m_triedLoadOnce = false; 

    std::vector<NavDebugLine> m_navDebugLines;
    bool m_drawNavMesh = false;

    bool m_hasPathStart = false;
    bool m_hasPathEnd = false;
    Vector3 m_pathStart{};
    Vector3 m_pathEnd{};
    std::vector<Vector3> m_debugPathPoints;

    std::vector<NavModifierVolumeData> m_modifierVolumes;
    NavAgentProfile m_debugPathProfile = NavAgentProfile::PlayerNormal;

    bool computeDebugPath(NavAgentProfile profile);
    std::vector<NavModifierVolumeData> collectNavModifierVolumes(Scene& scene) const;
    bool isPathBlockedByRuntimeBlockers(const std::vector<Vector3>& path) const;
};

