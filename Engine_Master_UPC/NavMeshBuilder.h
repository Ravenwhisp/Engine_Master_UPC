#pragma once
#include "NavMeshGeometryExtractor.h"

struct dtNavMesh;

class NavMeshBuilder
{
public:
	bool Build(const TriangleSoup& soup, dtNavMesh*& outMesh);
};