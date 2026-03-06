#pragma once
#include <vector>

class SceneModule;

struct TriangleSoup
{
	std::vector<float> vertices;
	std::vector<int> indices;
};

class NavMeshGeometryExtractor
{
public:
	static bool Extract(SceneModule& scene, TriangleSoup& out);
};