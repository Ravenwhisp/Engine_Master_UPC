#include "Globals.h"
#include "NavMeshGeometryExtractor.h"

#include "SceneModule.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "BasicMesh.h"

static void AppendMesh(
    const std::shared_ptr<BasicMesh> mesh,
    const Matrix& world,
    std::vector<float>& outVerts,
    std::vector<int>& outTris,
    int& inOutBaseVertex)
{
    if (!mesh) return;

    const auto& positions = mesh->getVertexPositions();
    const auto& indices = mesh->getIndices();

    if (positions.empty() || indices.empty())
        return;

    outVerts.reserve(outVerts.size() + positions.size() * 3);

    for (const Vector3& pLocal : positions)
    {
        const Vector3 pWorld = Vector3::Transform(pLocal, world);
        outVerts.push_back(pWorld.x);
        outVerts.push_back(pWorld.y);
        outVerts.push_back(pWorld.z);
    }

    outTris.reserve(outTris.size() + indices.size());
    for (uint8_t idx : indices)
        outTris.push_back(inOutBaseVertex + (int)idx);

    inOutBaseVertex += (int)positions.size();
}

static void CollectFromObject(
    GameObject* obj,
    std::vector<float>& outVerts,
    std::vector<int>& outTris,
    int& inOutBaseVertex,
    Layer requiredLayer,
    bool onlyActive)
{
    if (!obj) return;
    if (onlyActive && !obj->GetActive()) return;

    if (obj->GetLayer() == requiredLayer)
    {
        MeshRenderer* model = obj->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
        if (model)
        {
            const Matrix& world = obj->GetTransform()->getGlobalMatrix();
            const auto meshes = model->getMeshes();
            for (const auto mesh : meshes)
                AppendMesh(mesh, world, outVerts, outTris, inOutBaseVertex);
        }
    }

    for (GameObject* child : obj->GetTransform()->getAllChildren()) 
    {
        CollectFromObject(child, outVerts, outTris, inOutBaseVertex, requiredLayer, onlyActive);
    }
        
}

bool NavMeshGeometryExtractor::Extract(SceneModule& scene, TriangleSoup& out, Layer requiredLayer, bool onlyActive)
{
    out.vertices.clear();
    out.indices.clear();

    int baseVertex = 0;
    for (GameObject* root : scene.getAllGameObjects()) 
    {
        CollectFromObject(root, out.vertices, out.indices, baseVertex, requiredLayer, onlyActive);
    }
        

    return !out.vertices.empty() && !out.indices.empty();
}