#include "Globals.h"
#include "NavigationModule.h"
#include "Application.h"
#include "SceneModule.h"
#include "GameObject.h"
#include "Transform.h"
#include "ModelComponent.h"
#include "BasicMesh.h"

#include "NavMeshResource.h"

#include <fstream>
#include <vector>

#include <DetourNavMeshQuery.h>
#include <DetourAlloc.h>
#include <Logger.h>
#include <NavMeshBuilder.h>

static void AppendMeshToNavGeometry(
    const BasicMesh* mesh,
    const Matrix& world,
    std::vector<float>& outVerts,
    std::vector<int>& outTris,
    int& inOutBaseVertex)
{
    if (!mesh) return;

    const auto& positions = mesh->getCpuPositions();
    const auto& indices = mesh->getCpuIndices();  

    if (positions.empty() || indices.empty())
        return;

    // Add world-space vertices
    outVerts.reserve(outVerts.size() + positions.size() * 3);
    for (const Vector3& pLocal : positions)
    {
        const Vector3 pWorld = Vector3::Transform(pLocal, world);
        outVerts.push_back(pWorld.x);
        outVerts.push_back(pWorld.y);
        outVerts.push_back(pWorld.z);
    }

    // Add triangle indices with base offset
    outTris.reserve(outTris.size() + indices.size());
    for (uint32_t idx : indices) 
    {
        outTris.push_back(inOutBaseVertex + static_cast<int>(idx));
    }
        

    inOutBaseVertex += static_cast<int>(positions.size());
}

static void CollectNavGeometryFromGameObject(
    GameObject* obj,
    std::vector<float>& outVerts,
    std::vector<int>& outTris,
    int& inOutBaseVertex,
    Layer requiredLayer)
{
    if (!obj || !obj->GetActive())
        return;

    if (obj->GetLayer() == requiredLayer)
    {
        ModelComponent* model = obj->GetComponentAs<ModelComponent>(ComponentType::MODEL);
        if (model)
        {
            const Matrix& world = obj->GetTransform()->getGlobalMatrix();
            const auto meshes = model->getMeshes();

            for (const BasicMesh* mesh : meshes) 
            {
                AppendMeshToNavGeometry(mesh, world, outVerts, outTris, inOutBaseVertex);
            }
                
        }
    }

    for (GameObject* child : obj->GetTransform()->getAllChildren()) 
    {
        CollectNavGeometryFromGameObject(child, outVerts, outTris, inOutBaseVertex, requiredLayer);
    }
        
}

static void CollectNavGeometryFromScene(
    SceneModule* scene,
    std::vector<float>& outVerts,
    std::vector<int>& outTris,
    Layer requiredLayer)
{
    outVerts.clear();
    outTris.clear();

    if (!scene) return;

    int baseVertex = 0;
    for (GameObject* root : scene->getAllGameObjects()) 
    {
        CollectNavGeometryFromGameObject(root, outVerts, outTris, baseVertex, requiredLayer);
    }
        
}

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

    if (Logger::Instance())
    {
        std::vector<float> verts;
        std::vector<int> tris;

        CollectNavGeometryFromScene(app->getSceneModule(), verts, tris, Layer::NAVMESH);

        const int numVerts = (int)verts.size() / 3;
        const int numTris = (int)tris.size() / 3;

        LOG_INFO(__FILE__, __LINE__, "NavGeometry (Layer::NAVMESH): verts=%d tris=%d (vertsFloats=%zu trisInts=%zu)", numVerts, numTris, verts.size(), tris.size());
    }

    static bool builtOnce = false;
    if (!builtOnce && Logger::Instance())
    {
        builtOnce = true;
        buildNavMeshForCurrentScene();
    }

    return true;
}

void NavigationModule::update()
{
   

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

    rebuildNavMeshDebugLines();

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

bool NavigationModule::buildNavMeshForCurrentScene()
{
    if (!Logger::Instance()) return false;

    std::vector<float> verts;
    std::vector<int> tris;

    CollectNavGeometryFromScene(app->getSceneModule(), verts, tris, Layer::NAVMESH);

    const int numVerts = (int)verts.size() / 3;
    const int numTris = (int)tris.size() / 3;

    if (numVerts == 0 || numTris == 0)
    {
        LOG_WARNING(__FILE__, __LINE__, "NavMesh build aborted: no geometry in Layer::NAVMESH (verts=%d tris=%d).", numVerts, numTris);
        return false;
    }

    NavMeshBuildSettings settings; 
    NavMeshBuildResult result;

    if (!NavMeshBuilder::BuildSoloMesh(verts, tris, settings, result))
    {
        LOG_ERROR(__FILE__, __LINE__, "NavMesh build failed (Recast pipeline).");
        return false;
    }


    unloadNavMesh();
    m_navMesh = result.navMesh;
    m_navQuery = result.navQuery;
    m_tileRefs.clear();
    m_tileRefs.push_back(result.tileRef);

    const char* sceneName = app->getSceneModule()->getName();
    const bool saved = saveNavMeshForScene(sceneName);

    LOG_INFO(__FILE__, __LINE__, "NavMesh built: verts=%d tris=%d saved=%s", numVerts, numTris, saved ? "true" : "false");

    rebuildNavMeshDebugLines();

    return saved;
}

void NavigationModule::rebuildNavMeshDebugLines()
{
    m_navDebugLines.clear();
    if (!m_navMesh) return;

    // Border edges only (cleaner)
    for (dtTileRef ref : m_tileRefs)
    {
        const dtMeshTile* tile = m_navMesh->getTileByRef(ref);
        if (!tile || !tile->header) continue;

        const dtMeshHeader* h = tile->header;
        const float* tv = tile->verts;

        for (int i = 0; i < h->polyCount; ++i)
        {
            const dtPoly* p = &tile->polys[i];
            if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
                continue;

            const int nv = (int)p->vertCount;
            for (int j = 0; j < nv; ++j)
            {
                // skip internal edges
                if (p->neis[j] != 0)
                    continue;

                const unsigned short v0 = p->verts[j];
                const unsigned short v1 = p->verts[(j + 1) % nv];

                const float* a = &tv[v0 * 3];
                const float* b = &tv[v1 * 3];

                m_navDebugLines.push_back({
                    Vector3(a[0], a[1], a[2]),
                    Vector3(b[0], b[1], b[2])
                    });
            }
        }
    }
}

void NavigationModule::setPathStart(const Vector3& p)
{
    m_pathStart = p;
    m_hasPathStart = true;
    if (m_hasPathEnd) computeDebugPath();
}

void NavigationModule::setPathEnd(const Vector3& p)
{
    m_pathEnd = p;
    m_hasPathEnd = true;
    if (m_hasPathStart) computeDebugPath();
}

bool NavigationModule::computeDebugPath()
{
    m_debugPathPoints.clear();
    if (!m_navQuery || !m_navMesh) return false;
    if (!m_hasPathStart || !m_hasPathEnd) return false;

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    float ext[3] = { 2.0f, 4.0f, 2.0f }; 

    float s[3] = { m_pathStart.x, m_pathStart.y, m_pathStart.z };
    float e[3] = { m_pathEnd.x,   m_pathEnd.y,   m_pathEnd.z };

    dtPolyRef sRef = 0, eRef = 0;
    float sNearest[3], eNearest[3];

    if (dtStatusFailed(m_navQuery->findNearestPoly(s, ext, &filter, &sRef, sNearest)) || !sRef) return false;
    if (dtStatusFailed(m_navQuery->findNearestPoly(e, ext, &filter, &eRef, eNearest)) || !eRef) return false;

    dtPolyRef polys[256];
    int npolys = 0;
    if (dtStatusFailed(m_navQuery->findPath(sRef, eRef, sNearest, eNearest, &filter, polys, &npolys, 256)) || npolys == 0)
        return false;

    float straight[256 * 3];
    unsigned char straightFlags[256];
    dtPolyRef straightPolys[256];
    int nstraight = 0;

    if (dtStatusFailed(m_navQuery->findStraightPath(
        sNearest, eNearest, polys, npolys,
        straight, straightFlags, straightPolys,
        &nstraight, 256)))
        return false;

    m_debugPathPoints.reserve(nstraight);
    for (int i = 0; i < nstraight; ++i) 
    {
        m_debugPathPoints.emplace_back(straight[i * 3 + 0], straight[i * 3 + 1], straight[i * 3 + 2]);
    }
        

    return (m_debugPathPoints.size() >= 2);
}
