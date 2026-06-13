#include "Globals.h"
#include "NavMeshBuilder.h"

#include <cmath>
#include <cstring>

// Recast
#include <Recast.h>

// Detour
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourNavMeshBuilder.h>
#include <DetourAlloc.h>
#include <DetourTileCache.h>
#include <DetourTileCacheBuilder.h>

static int ClampInt(int v, int mn, int mx) { return v < mn ? mn : (v > mx ? mx : v); }

bool NavMeshBuilder::BuildSoloMesh(
    const std::vector<float>& verts,
    const std::vector<int>& tris,
    const NavMeshBuildSettings& s,
    NavMeshBuildResult& outResult,
    const std::vector<NavModifierVolumeData>& modifierVolumes)
{
    outResult = {};

    if (verts.empty() || tris.empty()) return false;
    if ((verts.size() % 3) != 0) return false;
    if ((tris.size() % 3) != 0) return false;

    const int nverts = (int)verts.size() / 3;
    const int ntris = (int)tris.size() / 3;

    // Recast expects vertex indices in [0, nverts)
    for (int i = 0; i < (int)tris.size(); ++i)
        if (tris[i] < 0 || tris[i] >= nverts) return false;

    rcContext ctx;

    rcConfig cfg{};
    cfg.cs = s.cellSize;
    cfg.ch = s.cellHeight;

    cfg.walkableSlopeAngle = s.agentMaxSlope;
    cfg.walkableHeight = (int)std::ceil(s.agentHeight / cfg.ch);
    cfg.walkableClimb = (int)std::floor(s.agentMaxClimb / cfg.ch);
    cfg.walkableRadius = (int)std::ceil(s.agentRadius / cfg.cs);

    cfg.maxEdgeLen = (int)(s.edgeMaxLen / cfg.cs);
    cfg.maxSimplificationError = s.edgeMaxError;

    cfg.minRegionArea = (int)rcSqr((int)s.regionMinSize);
    cfg.mergeRegionArea = (int)rcSqr((int)s.regionMergeSize);

    cfg.maxVertsPerPoly = ClampInt(s.vertsPerPoly, 3, 12);

    cfg.detailSampleDist = (s.detailSampleDist < 0.9f) ? 0 : cfg.cs * s.detailSampleDist;
    cfg.detailSampleMaxError = cfg.ch * s.detailSampleMaxError;

    // Compute bounds
    rcVcopy(cfg.bmin, &verts[0]);
    rcVcopy(cfg.bmax, &verts[0]);
    for (int i = 1; i < nverts; ++i)
    {
        const float* v = &verts[i * 3];
        rcVmin(cfg.bmin, v);
        rcVmax(cfg.bmax, v);
    }

    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

    // 1) Heightfield
    rcHeightfield* solid = rcAllocHeightfield();
    if (!solid) return false;

    if (!rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
    {
        rcFreeHeightField(solid);
        return false;
    }

    // 2) Mark walkable triangles by slope
    unsigned char* triAreas = (unsigned char*)dtAlloc(sizeof(unsigned char) * ntris, DT_ALLOC_TEMP);
    if (!triAreas)
    {
        rcFreeHeightField(solid);
        return false;
    }
    std::memset(triAreas, 0, sizeof(unsigned char) * ntris);

    rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts.data(), nverts, tris.data(), ntris, triAreas);

    // 3) Resolve Area For Point
    unsigned int defaultTriangles = 0;
    unsigned int spectralTriangles = 0;
    unsigned int blockedTriangles = 0;

    for (int i = 0; i < ntris; ++i)
    {
        const int i0 = tris[i * 3 + 0];
        const int i1 = tris[i * 3 + 1];
        const int i2 = tris[i * 3 + 2];

        Vector3 v0(
            verts[i0 * 3 + 0],
            verts[i0 * 3 + 1],
            verts[i0 * 3 + 2]
        );

        Vector3 v1(
            verts[i1 * 3 + 0],
            verts[i1 * 3 + 1],
            verts[i1 * 3 + 2]
        );

        Vector3 v2(
            verts[i2 * 3 + 0],
            verts[i2 * 3 + 1],
            verts[i2 * 3 + 2]
        );

        Vector3 center = (v0 + v1 + v2) / 3.0f;

        NavAreaType area = resolveAreaForPoint(center, modifierVolumes);

        if (area == NavAreaType::Default)
            defaultTriangles++;
        if (area == NavAreaType::Spectral)
            spectralTriangles++;
        if (area == NavAreaType::Blocked)
            blockedTriangles++;

        triAreas[i] = toRecastAreaId(area);
    }

    // 4) Rasterize triangles
    if (!rcRasterizeTriangles(&ctx, verts.data(), nverts, tris.data(), triAreas, ntris, *solid, cfg.walkableClimb))
    {
        dtFree(triAreas);
        rcFreeHeightField(solid);
        return false;
    }

    dtFree(triAreas);

    // 5) Filters
    rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
    rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

    // 6) Compact heightfield
    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    if (!chf)
    {
        rcFreeHeightField(solid);
        return false;
    }

    if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf))
    {
        rcFreeCompactHeightfield(chf);
        rcFreeHeightField(solid);
        return false;
    }

    rcFreeHeightField(solid);
    solid = nullptr;

    // 7) Erode by agent radius
    if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
    {
        rcFreeCompactHeightfield(chf);
        return false;
    }

    // 8) Regions
    if (!rcBuildDistanceField(&ctx, *chf))
    {
        rcFreeCompactHeightfield(chf);
        return false;
    }

    if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
    {
        rcFreeCompactHeightfield(chf);
        return false;
    }

    // 9) Contours
    rcContourSet* cset = rcAllocContourSet();
    if (!cset)
    {
        rcFreeCompactHeightfield(chf);
        return false;
    }

    if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
    {
        rcFreeContourSet(cset);
        rcFreeCompactHeightfield(chf);
        return false;
    }

    // 10) Poly mesh
    rcPolyMesh* pmesh = rcAllocPolyMesh();
    if (!pmesh)
    {
        rcFreeContourSet(cset);
        rcFreeCompactHeightfield(chf);
        return false;
    }

    if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
    {
        rcFreePolyMesh(pmesh);
        rcFreeContourSet(cset);
        rcFreeCompactHeightfield(chf);
        return false;
    }

    // 11) Detail mesh
    rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
    if (!dmesh)
    {
        rcFreePolyMesh(pmesh);
        rcFreeContourSet(cset);
        rcFreeCompactHeightfield(chf);
        return false;
    }

    if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
    {
        rcFreePolyMeshDetail(dmesh);
        rcFreePolyMesh(pmesh);
        rcFreeContourSet(cset);
        rcFreeCompactHeightfield(chf);
        return false;
    }

    rcFreeContourSet(cset);
    rcFreeCompactHeightfield(chf);
    cset = nullptr;
    chf = nullptr;

    // 12) Set Detour flags (simple: everything walkable => flag 1)
    for (int i = 0; i < pmesh->npolys; ++i)
    {
        // pmesh->flags[i] = 1;   --- old way
        pmesh->flags[i] = toPolyFlags(
            static_cast<NavAreaId>(pmesh->areas[i])
        );
    }

    // 13) Create Detour navmesh data
    dtNavMeshCreateParams params{};
    params.verts = pmesh->verts;
    params.vertCount = pmesh->nverts;
    params.polys = pmesh->polys;
    params.polyAreas = pmesh->areas;
    params.polyFlags = pmesh->flags;
    params.polyCount = pmesh->npolys;
    params.nvp = pmesh->nvp;

    params.detailMeshes = dmesh->meshes;
    params.detailVerts = dmesh->verts;
    params.detailVertsCount = dmesh->nverts;
    params.detailTris = dmesh->tris;
    params.detailTriCount = dmesh->ntris;

    params.walkableHeight = s.agentHeight;
    params.walkableRadius = s.agentRadius;
    params.walkableClimb = s.agentMaxClimb;

    rcVcopy(params.bmin, pmesh->bmin);
    rcVcopy(params.bmax, pmesh->bmax);
    params.cs = cfg.cs;
    params.ch = cfg.ch;
    params.buildBvTree = true;

    unsigned char* navData = nullptr;
    int navDataSize = 0;
    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
        rcFreePolyMeshDetail(dmesh);
        rcFreePolyMesh(pmesh);
        return false;
    }

    rcFreePolyMeshDetail(dmesh);
    rcFreePolyMesh(pmesh);
    dmesh = nullptr;
    pmesh = nullptr;

    // 14) Create dtNavMesh and add tile (so we get a tileRef for your Save())
    dtNavMesh* navMesh = dtAllocNavMesh();
    if (!navMesh)
    {
        dtFree(navData);
        return false;
    }

    dtNavMeshParams navParams{};
    rcVcopy(navParams.orig, params.bmin);

    // Single tile covers whole bounds (x,z)
    navParams.tileWidth = params.bmax[0] - params.bmin[0];
    navParams.tileHeight = params.bmax[2] - params.bmin[2];
    navParams.maxTiles = 1;
    navParams.maxPolys = params.polyCount;

    if (dtStatusFailed(navMesh->init(&navParams)))
    {
        dtFreeNavMesh(navMesh);
        dtFree(navData);
        return false;
    }

    dtTileRef outRef = 0;
    const dtStatus addSt = navMesh->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, &outRef);
    if (dtStatusFailed(addSt) || !outRef)
    {
        // If addTile fails, Detour will not own navData. Free it.
        dtFree(navData);
        dtFreeNavMesh(navMesh);
        return false;
    }

    dtNavMeshQuery* navQuery = dtAllocNavMeshQuery();
    if (!navQuery || dtStatusFailed(navQuery->init(navMesh, 2048)))
    {
        if (navQuery) dtFreeNavMeshQuery(navQuery);
        dtFreeNavMesh(navMesh); // this will free tile data due to DT_TILE_FREE_DATA
        return false;
    }

    outResult.navMesh = navMesh;
    outResult.navQuery = navQuery;
    outResult.tileRefs.push_back(outRef);

    DEBUG_LOG("NavMesh Areas: Default Triangles - %d, Spectral Triangles - %d, Blocked Triangles - %d, Modifier Volumes - %d", defaultTriangles, spectralTriangles, blockedTriangles, modifierVolumes.size());

    return true;
}

bool NavMeshBuilder::BuildTiledMesh(
    const std::vector<float>& verts,
    const std::vector<int>& tris,
    const NavMeshBuildSettings& s,
    NavMeshBuildResult& outResult,
    const std::vector<NavModifierVolumeData>& modifierVolumes,
    dtTileCacheAlloc* tileCacheAlloc,
    dtTileCacheCompressor* tileCacheCompressor,
    dtTileCacheMeshProcess* tileCacheMeshProcess)
{
    outResult = {};

    if (verts.empty() || tris.empty())
    {
        return false;
    }

    if ((verts.size() % 3) != 0)
    {
        return false;
    }

    if ((tris.size() % 3) != 0)
    {
        return false;
    }

    if (!tileCacheAlloc || !tileCacheCompressor || !tileCacheMeshProcess)
    {
        return false;
    }

    const int nverts = (int)verts.size() / 3;
    const int ntris = (int)tris.size() / 3;

    // Recast expects vertex indices in [0, nverts)
    for (int i = 0; i < (int)tris.size(); ++i)
    {
        if (tris[i] < 0 || tris[i] >= nverts)
        {
            return false;
        }
    }

    rcContext ctx;

    rcConfig cfg{};
    cfg.cs = s.cellSize;
    cfg.ch = s.cellHeight;

    cfg.walkableSlopeAngle = s.agentMaxSlope;
    cfg.walkableHeight = (int)std::ceil(s.agentHeight / cfg.ch);
    cfg.walkableClimb = (int)std::floor(s.agentMaxClimb / cfg.ch);
    cfg.walkableRadius = (int)std::ceil(s.agentRadius / cfg.cs);

    cfg.tileSize = s.tileSize;
    cfg.borderSize = cfg.walkableRadius + 3;

    cfg.maxEdgeLen = (int)(s.edgeMaxLen / cfg.cs);
    cfg.maxSimplificationError = s.edgeMaxError;

    cfg.minRegionArea = (int)rcSqr((int)s.regionMinSize);
    cfg.mergeRegionArea = (int)rcSqr((int)s.regionMergeSize);

    cfg.maxVertsPerPoly = ClampInt(s.vertsPerPoly, 3, 12);

    cfg.detailSampleDist = (s.detailSampleDist < 0.9f) ? 0 : cfg.cs * s.detailSampleDist;

    cfg.detailSampleMaxError = cfg.ch * s.detailSampleMaxError;

    // Compute bounds
    rcVcopy(cfg.bmin, &verts[0]);
    rcVcopy(cfg.bmax, &verts[0]);

    for (int i = 1; i < nverts; ++i)
    {
        const float* v = &verts[i * 3];
        rcVmin(cfg.bmin, v);
        rcVmax(cfg.bmax, v);
    }

    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

    const int tileW = (cfg.width + cfg.tileSize - 1) / cfg.tileSize;
    const int tileH = (cfg.height + cfg.tileSize - 1) / cfg.tileSize;

    DEBUG_LOG("TileCache Grid: %d x %d", tileW, tileH);

    // Create dtNavMesh
    dtNavMesh* navMesh = dtAllocNavMesh();
    if (!navMesh)
    {
        return false;
    }

    dtNavMeshParams navParams{};
    rcVcopy(navParams.orig, cfg.bmin);

    navParams.tileWidth = cfg.tileSize * cfg.cs;
    navParams.tileHeight = cfg.tileSize * cfg.cs;
    navParams.maxTiles = tileW * tileH;
    navParams.maxPolys = 32768;

    if (dtStatusFailed(navMesh->init(&navParams)))
    {
        dtFreeNavMesh(navMesh);
        return false;
    }

    // Initialize tileCache
    dtTileCacheParams tcparams{};
    rcVcopy(tcparams.orig, cfg.bmin);

    tcparams.cs = cfg.cs;
    tcparams.ch = cfg.ch;

    tcparams.width = cfg.tileSize;
    tcparams.height = cfg.tileSize;

    tcparams.walkableHeight = s.agentHeight;
    tcparams.walkableRadius = s.agentRadius;
    tcparams.walkableClimb = s.agentMaxClimb;

    tcparams.maxSimplificationError = s.edgeMaxError;
    
    tcparams.maxTiles = tileW * tileH;

    tcparams.maxObstacles = 128;

    dtTileCache* tileCache = dtAllocTileCache();
    if (!tileCache)
    {
        dtFreeNavMesh(navMesh);
        return false;
    }

    const dtStatus tcStatus = tileCache->init(
        &tcparams,
        tileCacheAlloc,
        tileCacheCompressor,
        tileCacheMeshProcess
    );

    if (dtStatusFailed(tcStatus))
    {
        dtFreeTileCache(tileCache);
        dtFreeNavMesh(navMesh);
        return false;
    }

    // Tile loop
    for (int y = 0; y < tileH; ++y)
    {
        for (int x = 0; x < tileW; ++x)
        {
            float tileBmin[3];
            float tileBmax[3];

            tileBmin[0] = cfg.bmin[0] + x * cfg.tileSize * cfg.cs;
            tileBmin[1] = cfg.bmin[1];
            tileBmin[2] = cfg.bmin[2] + y * cfg.tileSize * cfg.cs;

            tileBmax[0] = cfg.bmin[0] + (x + 1) * cfg.tileSize * cfg.cs;
            tileBmax[1] = cfg.bmax[1];
            tileBmax[2] = cfg.bmin[2] + (y + 1) * cfg.tileSize * cfg.cs;

            rcConfig tileCfg = cfg;

            rcVcopy(tileCfg.bmin, tileBmin);
            rcVcopy(tileCfg.bmax, tileBmax);

            tileCfg.bmin[0] -= tileCfg.borderSize * tileCfg.cs;
            tileCfg.bmin[2] -= tileCfg.borderSize * tileCfg.cs;

            tileCfg.bmax[0] += tileCfg.borderSize * tileCfg.cs;
            tileCfg.bmax[2] += tileCfg.borderSize * tileCfg.cs;

            tileCfg.width = tileCfg.tileSize + tileCfg.borderSize * 2;
            tileCfg.height = tileCfg.tileSize + tileCfg.borderSize * 2;

            // 1) Heightfield
            rcHeightfield* solid = rcAllocHeightfield();
            if (!solid)
            {
                continue;
            }

            if (!rcCreateHeightfield(
                &ctx,
                *solid,
                tileCfg.width,
                tileCfg.height,
                tileCfg.bmin,
                tileCfg.bmax,
                tileCfg.cs,
                tileCfg.ch))
            {
                rcFreeHeightField(solid);
                continue;
            }

            // 2) Triangle areas
            unsigned char* triAreas = (unsigned char*)dtAlloc(sizeof(unsigned char) * ntris, DT_ALLOC_TEMP);
            if (!triAreas)
            {
                rcFreeHeightField(solid);
                continue;
            }

            memset(triAreas, 0, sizeof(unsigned char) * ntris);

            // 3) Walkable triangles
            rcMarkWalkableTriangles(
                &ctx,
                tileCfg.walkableSlopeAngle,
                verts.data(),
                nverts,
                tris.data(),
                ntris,
                triAreas);

            // 4) Modifier Volumes
            for (int i = 0; i < ntris; ++i)
            {
                if (triAreas[i] == RC_NULL_AREA)
                {
                    continue;
                }

                const int i0 = tris[i * 3 + 0];
                const int i1 = tris[i * 3 + 1];
                const int i2 = tris[i * 3 + 2];

                Vector3 v0(
                    verts[i0 * 3 + 0],
                    verts[i0 * 3 + 1],
                    verts[i0 * 3 + 2]
                );

                Vector3 v1(
                    verts[i1 * 3 + 0],
                    verts[i1 * 3 + 1],
                    verts[i1 * 3 + 2]
                );

                Vector3 v2(
                    verts[i2 * 3 + 0],
                    verts[i2 * 3 + 1],
                    verts[i2 * 3 + 2]
                );

                Vector3 center = (v0 + v1 + v2) / 3.0f;

                NavAreaType area = resolveAreaForPoint(center, modifierVolumes);

                triAreas[i] = toRecastAreaId(area);
            }

            // 5) Rasterize
            if (!rcRasterizeTriangles(
                &ctx,
                verts.data(),
                nverts,
                tris.data(),
                triAreas,
                ntris,
                *solid,
                tileCfg.walkableClimb))
            {
                dtFree(triAreas);
                rcFreeHeightField(solid);
                continue;
            }

            dtFree(triAreas);

            // 6) Filters
            rcFilterLowHangingWalkableObstacles(
                &ctx,
                tileCfg.walkableClimb,
                *solid);

            rcFilterLedgeSpans(
                &ctx,
                tileCfg.walkableHeight,
                tileCfg.walkableClimb,
                *solid);

            rcFilterWalkableLowHeightSpans(
                &ctx,
                tileCfg.walkableHeight,
                *solid);

            // 7) Compact heightfield
            rcCompactHeightfield* chf = rcAllocCompactHeightfield();
            if (!chf)
            {
                rcFreeHeightField(solid);
                continue;
            }

            if (!rcBuildCompactHeightfield(
                &ctx,
                tileCfg.walkableHeight,
                tileCfg.walkableClimb,
                *solid,
                *chf))
            {
                rcFreeCompactHeightfield(chf);
                rcFreeHeightField(solid);
                continue;
            }

            // 8) Free solid
            rcFreeHeightField(solid);
            solid = nullptr;

            // 9) Erode
            if (!rcErodeWalkableArea(
                &ctx,
                tileCfg.walkableRadius,
                *chf))
            {
                rcFreeCompactHeightfield(chf);
                continue;
            }

            // 10) Heightfield layer
            rcHeightfieldLayerSet* lset = rcAllocHeightfieldLayerSet();
            if (!lset)
            {
                rcFreeCompactHeightfield(chf);
                continue;
            }

            if (!rcBuildHeightfieldLayers(
                &ctx,
                *chf,
                tileCfg.borderSize,
                tileCfg.walkableHeight,
                *lset))
            {
                rcFreeHeightfieldLayerSet(lset);
                rcFreeCompactHeightfield(chf);
                continue;
            }

            // 11) Build TileCache layer data

            for (int layerIndex = 0; layerIndex < lset->nlayers; ++layerIndex)
            {
                unsigned char* tileData = nullptr;
                int tileDataSize = 0;

                if (!BuildTileCacheLayer(
                    ctx,
                    tileCfg,
                    *chf,
                    x,
                    y,
                    layerIndex,
                    tileCacheCompressor,
                    &tileData,
                    &tileDataSize))
                {
                    continue;
                }

                dtCompressedTileRef tileRef = 0;

                const dtStatus addStatus =
                    tileCache->addTile(
                        tileData,
                        tileDataSize,
                        DT_COMPRESSEDTILE_FREE_DATA,
                        &tileRef);

                if (dtStatusFailed(addStatus))
                {
                    dtFree(tileData);
                    continue;
                }

                const dtStatus buildStatus =
                    tileCache->buildNavMeshTile(
                        tileRef,
                        navMesh);

                const dtCompressedTile* compressedTile = tileCache->getTileByRef(tileRef);

                if (compressedTile && compressedTile->header)
                {
                    dtTileRef navTileRef =
                        navMesh->getTileRefAt(
                            compressedTile->header->tx,
                            compressedTile->header->ty,
                            compressedTile->header->tlayer);

                    if (navTileRef)
                    {
                        outResult.tileRefs.push_back(navTileRef);
                    }
                }

                if (dtStatusFailed(buildStatus))
                {
                    continue;
                }
            }

            rcFreeHeightfieldLayerSet(lset);
            lset = nullptr;

            rcFreeCompactHeightfield(chf);
            chf = nullptr;
        }
    }

    dtNavMeshQuery* navQuery = dtAllocNavMeshQuery();

    if (!navQuery || dtStatusFailed(navQuery->init(navMesh, 2048)))
    {
        if (navQuery)
        {
            dtFreeNavMeshQuery(navQuery);
        }

        dtFreeTileCache(tileCache);
        dtFreeNavMesh(navMesh);
        return false;
    }

    outResult.navMesh = navMesh;
    outResult.navQuery = navQuery;
    outResult.tileCache = tileCache;

    return true;
}

bool NavMeshBuilder::BuildTileCacheLayer(
    rcContext& ctx,
    rcConfig& cfg,
    rcCompactHeightfield& chf,
    int tx,
    int ty,
    int tlayer,
    dtTileCacheCompressor* compressor,
    unsigned char** outData,
    int* outDataSize)
{
    *outData = nullptr;
    *outDataSize = 0;

    rcHeightfieldLayerSet* lset = rcAllocHeightfieldLayerSet();
    if (!lset)
    {
        return false;
    }

    if (!rcBuildHeightfieldLayers(
        &ctx,
        chf,
        cfg.borderSize,
        cfg.walkableHeight,
        *lset))
    {
        rcFreeHeightfieldLayerSet(lset);
        return false;
    }

    if (tlayer < 0 || tlayer >= lset->nlayers)
    {
        rcFreeHeightfieldLayerSet(lset);
        return false;
    }

    const rcHeightfieldLayer& layer = lset->layers[tlayer];

    dtTileCacheLayerHeader header{};
    header.magic = DT_TILECACHE_MAGIC;
    header.version = DT_TILECACHE_VERSION;

    header.tx = tx;
    header.ty = ty;
    header.tlayer = tlayer;

    rcVcopy(header.bmin, layer.bmin);
    rcVcopy(header.bmax, layer.bmax);

    header.width = (unsigned char)layer.width;
    header.height = (unsigned char)layer.height;
    header.minx = (unsigned char)layer.minx;
    header.maxx = (unsigned char)layer.maxx;
    header.miny = (unsigned char)layer.miny;
    header.maxy = (unsigned char)layer.maxy;
    header.hmin = (unsigned short)layer.hmin;
    header.hmax = (unsigned short)layer.hmax;

    dtStatus status = dtBuildTileCacheLayer(
        compressor,
        &header,
        layer.heights,
        layer.areas,
        layer.cons,
        outData,
        outDataSize
    );

    rcFreeHeightfieldLayerSet(lset);

    return dtStatusSucceed(status);
}

NavAreaType NavMeshBuilder::resolveAreaForPoint(
    const Vector3& point,
    const std::vector<NavModifierVolumeData>& modifierVolumes)
{
    NavAreaType result = NavAreaType::Default;
    int bestPriority = -9999;

    for (const auto& volume : modifierVolumes)
    {
        Vector3 min = volume.position - volume.halfExtents;
        Vector3 max = volume.position + volume.halfExtents;

        // AABB check if point is inside volume
        if (point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z)
        {
            if (volume.priority > bestPriority)
            {
                result = volume.areaType;
                bestPriority = volume.priority;
            }
        }
    }

    return result;
}

unsigned char NavMeshBuilder::toRecastAreaId(NavAreaType areaType)
{
    if (areaType == NavAreaType::Default)
        return static_cast<unsigned char>(NavAreaId::NAV_AREA_DEFAULT);
    else if (areaType == NavAreaType::Spectral)
        return static_cast<unsigned char>(NavAreaId::NAV_AREA_SPECTRAL);

    return RC_NULL_AREA;
}

unsigned short NavMeshBuilder::toPolyFlags(NavAreaId areaId)
{
    if (areaId == NavAreaId::NAV_AREA_DEFAULT)
        return static_cast<unsigned short>(NavPolyFlags::Default);
    else if (areaId == NavAreaId::NAV_AREA_SPECTRAL)
        return static_cast<unsigned short>(NavPolyFlags::Spectral);

    return 0;
}
