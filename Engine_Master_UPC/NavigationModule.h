#pragma once
#include "Module.h"


#include <vector>
#include <string>
#include <DetourNavMesh.h> // dtTileRef

struct dtNavMesh;
struct dtNavMeshQuery;

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

private:
    dtNavMesh* m_navMesh = nullptr;
    dtNavMeshQuery* m_navQuery = nullptr;
    std::vector<dtTileRef> m_tileRefs;

    std::string m_loadedScene;
    bool m_triedLoadOnce = false; 

};

