#pragma once

#include <vector>
#include <cstdint>
#include <DetourNavMesh.h>
#include "NavMeshTypes.h"
#include "NavMeshBuildSettings.h"

class dtNavMesh;
class dtNavMeshQuery;

struct NavMeshBuildResult
{
    dtNavMesh* navMesh = nullptr;
    dtNavMeshQuery* navQuery = nullptr;
    std::vector<dtTileRef> tileRefs;
};

class NavMeshBuilder
{
public:
    // verts: [x,y,z,x,y,z...], tris: [i0,i1,i2,i0,i1,i2...]
    static bool BuildSoloMesh(
        const std::vector<float>& verts,
        const std::vector<int>& tris,
        const NavMeshBuildSettings& settings,
        NavMeshBuildResult& outResult,
        const std::vector<NavModifierVolumeData>& modifierVolumes);

    static bool BuildTiledMesh(
        const std::vector<float>& verts,
        const std::vector<int>& tris,
        const NavMeshBuildSettings& settings,
        NavMeshBuildResult& outResult,
        const std::vector<NavModifierVolumeData>& modifierVolumes);

private:
    static NavAreaType resolveAreaForPoint(
        const Vector3& point,
        const std::vector<NavModifierVolumeData>& modifierVolumes
    );

private:
    // Helpers
    static unsigned char toRecastAreaId(NavAreaType areaType);
    static unsigned short toPolyFlags(NavAreaId areaId);
};

