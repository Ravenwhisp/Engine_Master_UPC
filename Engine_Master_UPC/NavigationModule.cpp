#include "Globals.h"
#include "NavigationModule.h"
#include "Application.h"
#include "SceneModule.h"

#include "NavMeshResource.h"
#include "NavMeshGeometryExtractor.h"

#include <fstream>

#include <DetourNavMeshQuery.h>
#include <DetourAlloc.h>

static std::string MakeNavMeshPath(const char* sceneName)
{
    return std::string("Assets/NavMeshes/") + sceneName + ".navmesh";
}

bool NavigationModule::init()
{
	return true;
}

bool NavigationModule::postInit()
{
    const char* sceneName = app->getSceneModule()->getName();
    m_triedLoadOnce = true;
    loadNavMeshForScene(sceneName);
    return true;
}

void NavigationModule::update()
{
    // Used for TESTING ONLY
    if (GetAsyncKeyState(VK_F9) & 1)
    {
        TriangleSoup soup;

        if (NavMeshGeometryExtractor::Extract(*app->getSceneModule(), soup))
        {
            DEBUG_LOG("NavMesh geometry: %zu vertices, %zu indices\n",
                soup.vertices.size() / 3,
                soup.indices.size() / 3);
        }
        else
        {
            DEBUG_ERROR("NavMesh geometry extraction FAILED\n");
        }
    }
}

bool NavigationModule::cleanUp()
{
    unloadNavMesh();
    return true;
}

bool NavigationModule::loadNavMeshForScene(const char* sceneName)
{
    unloadNavMesh();
    m_tileRefs.clear();

    const std::string path = MakeNavMeshPath(sceneName);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        m_loadedScene.clear();
        return false;
    }

    NavMeshSetHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file || header.magic != NAVMESHSET_MAGIC || header.version != NAVMESHSET_VERSION)
        return false;

    dtNavMesh* mesh = dtAllocNavMesh();
    if (!mesh) return false;

    if (dtStatusFailed(mesh->init(&header.params)))
    {
        dtFreeNavMesh(mesh);
        return false;
    }

    for (int i = 0; i < header.numTiles; ++i)
    {
        NavMeshTileHeader tileHeader{};
        file.read(reinterpret_cast<char*>(&tileHeader), sizeof(tileHeader));
        if (!file || !tileHeader.tileRef || !tileHeader.dataSize)
        {
            dtFreeNavMesh(mesh);
            return false;
        }

        unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
        if (!data)
        {
            dtFreeNavMesh(mesh);
            return false;
        }

        file.read(reinterpret_cast<char*>(data), tileHeader.dataSize);
        if (!file)
        {
            dtFree(data);
            dtFreeNavMesh(mesh);
            return false;
        }

        const dtStatus st = mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, nullptr);
        if (dtStatusFailed(st))
        {
            dtFree(data);
            dtFreeNavMesh(mesh);
            return false;
        }

        m_tileRefs.push_back(tileHeader.tileRef);
    }

    dtNavMeshQuery* query = dtAllocNavMeshQuery();
    if (!query || dtStatusFailed(query->init(mesh, 2048)))
    {
        if (query) dtFreeNavMeshQuery(query);
        dtFreeNavMesh(mesh);
        return false;
    }

    m_navMesh = mesh;
    m_navQuery = query;
    m_loadedScene = sceneName;
    return true;
}

bool NavigationModule::unloadNavMesh()
{
    if (m_navQuery) { dtFreeNavMeshQuery(m_navQuery); m_navQuery = nullptr; }
    if (m_navMesh) { dtFreeNavMesh(m_navMesh);       m_navMesh = nullptr; }
    m_tileRefs.clear();
    m_loadedScene.clear();
    return true;
}

bool NavigationModule::saveNavMeshForScene(const char* sceneName) const
{
    if (!m_navMesh || !sceneName) return false;

    const std::string path = MakeNavMeshPath(sceneName);
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    NavMeshSetHeader header{};
    header.magic = NAVMESHSET_MAGIC;
    header.version = NAVMESHSET_VERSION;
    header.numTiles = (int)m_tileRefs.size();
    header.params = *m_navMesh->getParams();

    file.write((const char*)&header, sizeof(header));
    if (!file) return false;

    for (dtTileRef ref : m_tileRefs)
    {
        const dtMeshTile* tile = m_navMesh->getTileByRef(ref);
        if (!tile || !tile->header || !tile->data || tile->dataSize <= 0)
            return false;

        NavMeshTileHeader th{};
        th.tileRef = ref;
        th.dataSize = tile->dataSize;

        file.write((const char*)&th, sizeof(th));
        if (!file) return false;

        file.write((const char*)tile->data, tile->dataSize);
        if (!file) return false;
    }

    return true;
}