#include "Globals.h"
#include "ModuleNavigation.h"

#include "Application.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "BasicMesh.h"
#include "NavMeshResource.h"

#include <fstream>
#include <vector>

#include <DetourNavMeshQuery.h>
#include <DetourAlloc.h>
#include <DetourTileCache.h>
#include <DetourTileCacheBuilder.h>
#include <WindowLogger.h>
#include <NavMeshBuilder.h>

#include "NavMeshGeometryExtractor.h"
#include "NavModifierVolumeComponent.h"

static std::string MakeNavMeshPath(const char* sceneName)
{
    return std::string("Assets/NavMeshes/") + sceneName + ".navmesh";
}

// Detour needs custom allocator for dtTileCache to work (linear allocator is used in the recast demo)
struct LinearAllocator : public dtTileCacheAlloc
{
    unsigned char* buffer = nullptr; // memory block [..............]
    size_t capacity = 0; // memory size
    size_t top = 0; // current memory [###............]

    LinearAllocator(const size_t cap)
    {
        capacity = cap;

        // allocate one big chunk of memory
        buffer = (unsigned char*)dtAlloc(capacity, DT_ALLOC_PERM);
    }

    ~LinearAllocator() override
    {
        dtFree(buffer);
    }

    void reset() override
    {
        top = 0; // not deleting memory just reusing slots
    }

    void* alloc(const size_t size) override
    {
        if (!buffer)
        {
            return nullptr;
        }

        if (top + size > capacity)
        {
            return nullptr;
        }

        unsigned char* mem = buffer + top;
        top += size;

        return mem;
    }

    void free(void*) override
    {
        // we don't free individual allocations
        // memory is reused when reset() is called
        // interface requires overriding this func
    }
};

// Compresses tile data
struct FastLZCompressor : public dtTileCacheCompressor
{
    int maxCompressedSize(const int bufferSize) override
    {
        return bufferSize; // not compressing currently - just for testing
    }

    dtStatus compress(
        const unsigned char* buffer,
        const int bufferSize,
        unsigned char* compressed,
        const int maxCompressedSize,
        int* compressedSize
    ) override
    {
        if (maxCompressedSize < bufferSize)
        {
            return DT_FAILURE;
        }

        memcpy(compressed, buffer, bufferSize);
        *compressedSize = bufferSize;

        return DT_SUCCESS;
    }

    dtStatus decompress(
        const unsigned char* compressed,
        const int compressedSize,
        unsigned char* buffer,
        const int maxBufferSize,
        int* bufferSize
    ) override
    {
        if (maxBufferSize < compressedSize)
        {
            return DT_FAILURE;
        }

        memcpy(buffer, compressed, compressedSize);
        *bufferSize = compressedSize;

        return DT_SUCCESS;
    }
};

bool ModuleNavigation::init()
{
    const char* sceneName = app->getModuleScene()->getScene()->getName();
    m_triedLoadOnce = true;
    loadNavMeshForScene(sceneName);

    if (WindowLogger::Instance())
    {
        TriangleSoup soup;
        NavMeshGeometryExtractor::Extract(*app->getModuleScene()->getScene(), soup, Layer::NAVMESH, true);

        const auto& verts = soup.vertices;
        const auto& tris = soup.indices;

        const int numVerts = (int)verts.size() / 3;
        const int numTris = (int)tris.size() / 3;

        LOG_INFO(__FILE__, __LINE__, "NavGeometry (Layer::NAVMESH): verts=%d tris=%d (vertsFloats=%zu trisInts=%zu)", numVerts, numTris, verts.size(), tris.size());
    }

	return true;
}

bool ModuleNavigation::cleanUp()
{
    unloadNavMesh();
    return true;
}

bool ModuleNavigation::loadNavMeshForScene(const char* sceneName)
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

bool ModuleNavigation::unloadNavMesh()
{
    if (m_navQuery) { dtFreeNavMeshQuery(m_navQuery); m_navQuery = nullptr; }
    if (m_navMesh) { dtFreeNavMesh(m_navMesh);       m_navMesh = nullptr; }
    if (m_tileCache) { dtFreeTileCache(m_tileCache); m_tileCache = nullptr; }
    m_tileRefs.clear();
    m_loadedScene.clear();
    return true;
}

bool ModuleNavigation::saveNavMeshForScene(const char* sceneName) const
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

bool ModuleNavigation::buildNavMeshForCurrentScene()
{
    if (!WindowLogger::Instance()) return false;

    TriangleSoup soup;
    NavMeshGeometryExtractor::Extract(*app->getModuleScene()->getScene(), soup, Layer::NAVMESH, true);

    const auto& verts = soup.vertices;
    const auto& tris = soup.indices;

    const int numVerts = (int)verts.size() / 3;
    const int numTris = (int)tris.size() / 3;

    if (numVerts == 0 || numTris == 0)
    {
        LOG_WARNING(__FILE__, __LINE__, "NavMesh build aborted: no geometry in Layer::NAVMESH (verts=%d tris=%d).", numVerts, numTris);
        return false;
    }

    NavMeshBuildSettings settings;
    settings.cellSize = m_settings.cellSize;
    settings.cellHeight = m_settings.cellHeight;
    settings.tileSize = m_settings.tileSize;
    settings.agentHeight = m_settings.agentHeight;
    settings.agentRadius = m_settings.agentRadius;
    settings.agentMaxClimb = m_settings.agentMaxClimb;
    settings.agentMaxSlope = m_settings.agentMaxSlope;

    NavMeshBuildResult result;

    // get modifier volumes from the scene
    m_modifierVolumes = collectNavModifierVolumes(*app->getModuleScene()->getScene());

    if (!NavMeshBuilder::BuildTiledMesh(verts, tris, settings, result, m_modifierVolumes))
    {
        LOG_ERROR(__FILE__, __LINE__, "NavMesh build failed (Recast pipeline).");
        return false;
    }


    unloadNavMesh();
    m_navMesh = result.navMesh;
    m_navQuery = result.navQuery;
    m_tileRefs = result.tileRefs;

    const char* sceneName = app->getModuleScene()->getScene()->getName();
    const bool saved = saveNavMeshForScene(sceneName);

    LOG_INFO(__FILE__, __LINE__, "NavMesh built: verts=%d tris=%d saved=%s", numVerts, numTris, saved ? "true" : "false");

    rebuildNavMeshDebugLines();

    return saved;
}

void ModuleNavigation::rebuildNavMeshDebugLines()
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

                const float* color = dd::colors::White;
                
                if (p->flags & static_cast<unsigned short>(NavPolyFlags::Default))
                    color = dd::colors::Green;
                else if (p->flags & static_cast<unsigned short>(NavPolyFlags::Spectral))
                    color = dd::colors::Blue;
                else
                    color = dd::colors::Red; // for unknown flag

                m_navDebugLines.push_back({
                    Vector3(a[0], a[1], a[2]),
                    Vector3(b[0], b[1], b[2]),
                    color
                    });
            }
        }
    }
}

void ModuleNavigation::setPathStart(const Vector3& p, NavAgentProfile profile)
{
    m_pathStart = p;
    m_hasPathStart = true;
    if (m_hasPathEnd) computeDebugPath(profile);
}

void ModuleNavigation::setPathEnd(const Vector3& p, NavAgentProfile profile)
{
    m_pathEnd = p;
    m_hasPathEnd = true;
    if (m_hasPathStart) computeDebugPath(profile);
}

bool ModuleNavigation::findStraightPath(const Vector3& start, const Vector3& end, std::vector<Vector3>& outPath, const Vector3& extents, NavAgentProfile profile) const
{
    if (!m_navQuery)
        return false;

    float startPos[3] = { start.x, start.y, start.z };
    float endPos[3] = { end.x, end.y, end.z };

    float exts[3] = { extents.x, extents.y, extents.z };

    dtQueryFilter filter;
    
    unsigned short includeFlags = getIncludeFlagsForProfile(profile);
    filter.setIncludeFlags(includeFlags);
    filter.setExcludeFlags(0);

    dtPolyRef startRef = 0, endRef = 0;
    float nearestStart[3], nearestEnd[3];

    if (!dtStatusSucceed(m_navQuery->findNearestPoly(startPos, exts, &filter, &startRef, nearestStart)))
        return false;

    if (!dtStatusSucceed(m_navQuery->findNearestPoly(endPos, exts, &filter, &endRef, nearestEnd)))
        return false;

    dtPolyRef pathPolys[128];
    int pathCount = 0;

    if (!dtStatusSucceed(m_navQuery->findPath(
        startRef, endRef,
        nearestStart, nearestEnd,
        &filter,
        pathPolys, &pathCount, 128)))
        return false;

    float straight[128 * 3];
    unsigned char flags[128];
    dtPolyRef refs[128];
    int straightCount = 0;

    m_navQuery->findStraightPath(
        nearestStart, nearestEnd,
        pathPolys, pathCount,
        straight, flags, refs,
        &straightCount, 128);

    if (straightCount < 2)
        return false;

    outPath.clear();

    for (int i = 0; i < straightCount; ++i)
    {
        outPath.emplace_back(
            straight[i * 3 + 0],
            straight[i * 3 + 1],
            straight[i * 3 + 2]
        );
    }

    return true;
}

void ModuleNavigation::debugDraw()
{
    if (getDrawNavMesh() && getNavMesh())
    {
        for (const auto& l : getNavMeshDebugLines())
        {
            dd::line(ddConvert(l.a), ddConvert(l.b), l.color);

        }
    }

    if (hasDebugPath())
    {
        const auto& pts = getDebugPathPoints();
        for (size_t i = 1; i < pts.size(); ++i)
        {
            dd::line(ddConvert(pts[i - 1]), ddConvert(pts[i]), dd::colors::Yellow);
        }


        dd::line(ddConvert(pts.front()), ddConvert(pts.front() + Vector3(0, 0.25f, 0)), dd::colors::Yellow);
        dd::line(ddConvert(pts.back()), ddConvert(pts.back() + Vector3(0, 0.25f, 0)), dd::colors::Yellow);
    }
}

bool ModuleNavigation::computeDebugPath(NavAgentProfile profile)
{
    m_debugPathPoints.clear();
    if (!m_navQuery || !m_navMesh) return false;
    if (!m_hasPathStart || !m_hasPathEnd) return false;

    dtQueryFilter filter;
    //filter.setIncludeFlags(0xFFFF);
    filter.setIncludeFlags(getIncludeFlagsForProfile(profile));
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

std::vector<NavModifierVolumeData> ModuleNavigation::collectNavModifierVolumes(Scene& scene) const
{
    std::vector<NavModifierVolumeData> data;

    for (GameObject* obj : scene.getAllGameObjects())
    {
        NavModifierVolumeComponent* navComp = obj->GetComponentAs<NavModifierVolumeComponent>(ComponentType::NAVMODIFIER_VOLUME);
        if (navComp)
        {
            Transform* transformComp = obj->GetComponentAs<Transform>(ComponentType::TRANSFORM);
            if (transformComp)
            {
                // if component is enabled -> add it to the data
                if (navComp->getEnabled())
                {
                    NavModifierVolumeData volume;
                    volume.position = transformComp->getPosition();
                    volume.halfExtents = navComp->getHalfExtents();
                    volume.areaType = navComp->getAreaType();
                    volume.enabled = navComp->getEnabled();
                    volume.priority = navComp->getPriority();
                    data.push_back(volume);
                }
            }
        }
    }

    return data;
}

unsigned short ModuleNavigation::getIncludeFlagsForProfile(NavAgentProfile profile) const
{
    unsigned short defaultFlag = static_cast<unsigned short>(NavPolyFlags::Default);
    unsigned short spectralFlag = static_cast<unsigned short>(NavPolyFlags::Spectral);

    if (profile == NavAgentProfile::PlayerNormal)
        return defaultFlag;

    if (profile == NavAgentProfile::PlayerSpectral)
        return defaultFlag | spectralFlag;

    if (profile == NavAgentProfile::EnemyGround)
        return defaultFlag;

    return defaultFlag;
}