#pragma once
#include <vector>
#include "Layer.h"

class SceneModule;
class GameObject;

struct TriangleSoup
{
    std::vector<float> vertices;
    std::vector<int> indices;   
};

class NavMeshGeometryExtractor
{
public:
    static bool Extract(SceneModule& scene, TriangleSoup& out, Layer requiredLayer = Layer::NAVMESH, bool onlyActive = true);
};