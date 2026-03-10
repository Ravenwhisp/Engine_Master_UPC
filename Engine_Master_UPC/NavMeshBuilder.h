#pragma once

#include <vector>
#include <cstdint>
#include <DetourNavMesh.h>

struct dtNavMesh;
struct dtNavMeshQuery;

struct NavMeshBuildSettings
{
    float cellSize = 0.3f;
    float cellHeight = 0.2f;

    float agentHeight = 2.0f;
    float agentRadius = 0.6f;
    float agentMaxClimb = 0.9f;
    float agentMaxSlope = 45.0f;

    float regionMinSize = 8.0f;
    float regionMergeSize = 20.0f;

    float edgeMaxLen = 12.0f;
    float edgeMaxError = 1.3f;

    int vertsPerPoly = 6;

    float detailSampleDist = 6.0f;
    float detailSampleMaxError = 1.0f;
};

struct NavMeshBuildResult
{
    dtNavMesh* navMesh = nullptr;
    dtNavMeshQuery* navQuery = nullptr;
    dtTileRef tileRef = 0;
};

class NavMeshBuilder
{
public:
    // verts: [x,y,z,x,y,z...], tris: [i0,i1,i2,i0,i1,i2...]
    static bool BuildSoloMesh(
        const std::vector<float>& verts,
        const std::vector<int>& tris,
        const NavMeshBuildSettings& settings,
        NavMeshBuildResult& outResult);
};

