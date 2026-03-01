#include "Globals.h"
#include "NavMeshBuilder.h"

#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourAlloc.h>
#include <cmath>

bool NavMeshBuilder::Build(const TriangleSoup& soup, dtNavMesh*& outMesh)
{
	outMesh = nullptr;

	const int numberOfVertices = (int)soup.vertices.size() / 3;
	const int numberOfIndices = (int)soup.indices.size() / 3;

	if (numberOfVertices == 0 || numberOfIndices == 0)
		return false;

	rcContext ctx(false);

	// Agent settings for testing ONLY -- will be configurable in the future

	const float cellSize = 0.3f;
	const float cellHeight = 0.2f;

	const float agentHeight = 1.8f;
	const float agentRadius = 0.4f;
	const float agentMaxClimb = 0.6f;
	const float agentMaxSlope = 45.0f;

	// end of agent settings

	rcConfig cfg{};
	cfg.cs = cellSize;
	cfg.ch = cellHeight;

	cfg.walkableSlopeAngle = agentMaxSlope;
	cfg.walkableHeight = (int)std::ceil(agentHeight / cfg.ch);
	cfg.walkableClimb = (int)std::floor(agentMaxClimb / cfg.ch);
	cfg.walkableRadius = (int)std::ceil(agentRadius / cfg.cs);

	cfg.maxVertsPerPoly = 6;

	rcCalcBounds(soup.vertices.data(), numberOfVertices, cfg.bmin, cfg.bmax);
	
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	rcHeightfield* solid = rcAllocHeightfield();

	rcCreateHeightfield(
		&ctx,
		*solid,
		cfg.width,
		cfg.height,
		cfg.bmin,
		cfg.bmax,
		cfg.cs,
		cfg.ch
	);

	std::vector<unsigned char> areas(numberOfIndices);

	rcMarkWalkableTriangles(
		&ctx,
		cfg.walkableSlopeAngle,
		soup.vertices.data(),
		numberOfVertices,
		soup.indices.data(),
		numberOfIndices,
		areas.data()
	);

	rcRasterizeTriangles(
		&ctx,
		soup.vertices.data(),
		numberOfVertices,
		soup.indices.data(),
		areas.data(),
		numberOfIndices,
		*solid,
		cfg.walkableClimb
	);

	rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
	rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
	rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

	rcCompactHeightfield* chf = rcAllocCompactHeightfield();

	rcBuildCompactHeightfield(
		&ctx,
		cfg.walkableHeight,
		cfg.walkableClimb,
		*solid,
		*chf
	);

	rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf);

	rcBuildDistanceField(&ctx, *chf);
	rcBuildRegions(&ctx, *chf, 0, 8, 20);

	rcContourSet* cset = rcAllocContourSet();
	rcBuildContours(&ctx, *chf, 1.3f, 12.0f, *cset);

	rcPolyMesh* pmesh = rcAllocPolyMesh();
	rcBuildPolyMesh(&ctx, *cset, 6, *pmesh);

	rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
	rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, 6, 1, *dmesh);

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
	params.walkableHeight = agentHeight;
	params.walkableRadius = agentRadius;
	params.walkableClimb = agentMaxClimb;

	rcVcopy(params.bmin, pmesh->bmin);
	rcVcopy(params.bmax, pmesh->bmax);

	params.cs = cfg.cs;
	params.ch = cfg.ch;
	params.buildBvTree = true;

	unsigned char* navData = nullptr;
	int navDataSize = 0;

	if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		return false;

	dtNavMesh* navMesh = dtAllocNavMesh();

	if (!navMesh)
		return false;

	if (dtStatusFailed(navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA)))
		return false;

	outMesh = navMesh;

	return true;
}