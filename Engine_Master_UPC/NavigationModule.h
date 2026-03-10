#pragma once
#include "Module.h"


#include <vector>
#include <string>
#include <DetourNavMesh.h> // dtTileRef

struct dtNavMesh;
struct dtNavMeshQuery;

struct NavMeshSettings
{
    float cellSize = 0.3f;
    float cellHeight = 0.2f;

    float agentHeight = 1.8f;
    float agentRadius = 0.4f;

    float agentMaxClimb = 0.6f;
    float agentMaxSlope = 45.0f;
};

class NavigationModule : public Module
{
public:
    bool init() override;
    bool postInit() override;
    void update() override;
    bool cleanUp() override;

    // Access
    dtNavMesh* getNavMesh() const { return m_navMesh; }
    dtNavMeshQuery* getNavQuery() const { return m_navQuery; }
    const std::string& getLoadedScene() const { return m_loadedScene; }

    // Scene resource API
    bool loadNavMeshForScene(const char* sceneName);
    bool unloadNavMesh();
    bool saveNavMeshForScene(const char* sceneName) const;
    bool buildNavMeshForCurrentScene();

    // Debug
    struct NavDebugLine
    {
        Vector3 a;
        Vector3 b;
    };

    const std::vector<NavDebugLine>& getNavMeshDebugLines() const { return m_navDebugLines; }
    void rebuildNavMeshDebugLines();

    void setDrawNavMesh(bool v) { m_drawNavMesh = v; }
    bool getDrawNavMesh() const { return m_drawNavMesh; }

    NavMeshSettings& getSettings() { return m_settings; }
    bool hasNavMesh() const { return m_navMesh != nullptr && m_navQuery != nullptr; }

    void setPathStart(const Vector3& p);
    void setPathEnd(const Vector3& p);
    const std::vector<Vector3>& getDebugPathPoints() const { return m_debugPathPoints; }
    bool hasDebugPath() const { return m_debugPathPoints.size() >= 2; }
    bool findStraightPath(const Vector3& start, const Vector3& end, std::vector<Vector3>& outPath, const Vector3& extents) const;

private:
    NavMeshSettings m_settings;
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

    bool computeDebugPath();

};

