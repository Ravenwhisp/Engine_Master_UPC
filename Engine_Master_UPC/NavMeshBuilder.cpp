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

static int ClampInt(int v, int mn, int mx) { return v < mn ? mn : (v > mx ? mx : v); }

bool NavMeshBuilder::BuildSoloMesh(
    const std::vector<float>& verts,
    const std::vector<int>& tris,
    const NavMeshBuildSettings& s,
    NavMeshBuildResult& outResult)
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

    // 2) Mark walkable triangles by slope and rasterize
    unsigned char* triAreas = (unsigned char*)dtAlloc(sizeof(unsigned char) * ntris, DT_ALLOC_TEMP);
    if (!triAreas)
    {
        rcFreeHeightField(solid);
        return false;
    }
    std::memset(triAreas, 0, sizeof(unsigned char) * ntris);

    rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts.data(), nverts, tris.data(), ntris, triAreas);

    if (!rcRasterizeTriangles(&ctx, verts.data(), nverts, tris.data(), triAreas, ntris, *solid, cfg.walkableClimb))
    {
        dtFree(triAreas);
        rcFreeHeightField(solid);
        return false;
    }

    dtFree(triAreas);

    // 3) Filters
    rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
    rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

    // 4) Compact heightfield
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

    // 5) Erode by agent radius
    if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
    {
        rcFreeCompactHeightfield(chf);
        return false;
    }

    // 6) Regions
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

    // 7) Contours
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

    // 8) Poly mesh
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

    // 9) Detail mesh
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

    // 10) Set Detour flags (simple: everything walkable => flag 1)
    for (int i = 0; i < pmesh->npolys; ++i)
        pmesh->flags[i] = 1;

    // 11) Create Detour navmesh data
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

    // 12) Create dtNavMesh and add tile (so we get a tileRef for your Save())
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
    outResult.tileRef = outRef;
    return true;
}