#pragma once
#include <vector>
#include "Layer.h"

class Scene;

struct TriangleSoup
{
    std::vector<float> vertices;
    std::vector<int> indices;   
};

class NavMeshGeometryExtractor
{
public:
    static bool Extract(Scene& scene, TriangleSoup& out, Layer requiredLayer = Layer::NAVMESH, bool onlyActive = true);
};