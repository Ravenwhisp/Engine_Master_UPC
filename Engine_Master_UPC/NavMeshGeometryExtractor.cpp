#include "Globals.h"
#include "NavMeshGeometryExtractor.h"

#include "ModuleScene.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Application.h"
#include "ModuleAssets.h"
#include "ModelAsset.h"
#include <Logger.h>

static bool DecodeIndices(const MeshAsset& mesh, std::vector<uint32_t>& out)
{
    out.clear();

    const uint32_t indexCount = mesh.getIndexCount();
    if (indexCount == 0)
        return false;

    out.resize(indexCount);

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(mesh.getIndexData());
    if (!bytes)
        return false;

    switch (mesh.getIndexFormat())
    {
    case DXGI_FORMAT_R8_UINT:
        for (uint32_t i = 0; i < indexCount; ++i)
            out[i] = bytes[i];
        return true;

    case DXGI_FORMAT_R16_UINT:
    {
        const uint16_t* src = reinterpret_cast<const uint16_t*>(bytes);
        for (uint32_t i = 0; i < indexCount; ++i)
            out[i] = src[i];
        return true;
    }

    case DXGI_FORMAT_R32_UINT:
    {
        const uint32_t* src = reinterpret_cast<const uint32_t*>(bytes);
        for (uint32_t i = 0; i < indexCount; ++i)
            out[i] = src[i];
        return true;
    }

    default:
        DEBUG_WARN("[NavMesh] Unsupported index format in MeshAsset.");
        return false;
    }
}

static void AppendMeshAssetToSoup(
    const MeshAsset& mesh,
    const Matrix& world,
    std::vector<float>& outVerts,
    std::vector<int>& outTris,
    int& inOutBaseVertex)
{
    const uint32_t vCount = mesh.getVertexCount();
    if (vCount == 0)
        return;

    // ---- vertices ----
    const Vertex* verts = reinterpret_cast<const Vertex*>(mesh.getVertexData());
    if (!verts)
        return;

    outVerts.reserve(outVerts.size() + size_t(vCount) * 3);

    for (uint32_t i = 0; i < vCount; ++i)
    {
        const Vector3 pWorld = Vector3::Transform(verts[i].position, world);
        outVerts.push_back(pWorld.x);
        outVerts.push_back(pWorld.y);
        outVerts.push_back(pWorld.z);
    }

    // ---- indices (decode) ----
    std::vector<uint32_t> decoded;
    if (!DecodeIndices(mesh, decoded))
    {
        if ((vCount % 3) == 0) { for (uint32_t i = 0; i < vCount; ++i) outTris.push_back(inOutBaseVertex + int(i)); }
        return;
    }

    if ((decoded.size() % 3) != 0)
    {
        DEBUG_WARN("[NavMesh] Index count not multiple of 3 (%zu). Truncating.", decoded.size());
        decoded.resize(decoded.size() - (decoded.size() % 3));
    }

    for (uint32_t idx : decoded)
    {
        if (idx >= vCount)
        {
            DEBUG_WARN("[NavMesh] Invalid index %u (vCount=%u). Skipping mesh.", idx, vCount);
            return;
        }
    }

    outTris.reserve(outTris.size() + decoded.size());
    for (uint32_t idx : decoded)
        outTris.push_back(inOutBaseVertex + static_cast<int>(idx));

    inOutBaseVertex += static_cast<int>(vCount);
}

static void CollectFromObject(
    GameObject* obj,
    std::vector<float>& outVerts,
    std::vector<int>& outTris,
    int& inOutBaseVertex,
    Layer requiredLayer,
    bool onlyActive)
{
    if (!obj)
        return;

    if (onlyActive && !obj->GetActive())
        return;

    if (obj->GetLayer() == requiredLayer)
    {
        MeshRenderer* renderer = obj->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
        if (renderer)
        {
            const UID modelId = renderer->getModelAssetId();
            if (modelId != 0)
            {
                ModelAsset* model = static_cast<ModelAsset*>(app->getAssetModule()->requestAsset(modelId));
                if (model)
                {
                    const Matrix world = obj->GetTransform()->getGlobalMatrix();
                    for (const MeshAsset& mesh : model->getMeshes())
                        AppendMeshAssetToSoup(mesh, world, outVerts, outTris, inOutBaseVertex);
                }
            }
        }
    }

    for (GameObject* child : obj->GetTransform()->getAllChildren())
        CollectFromObject(child, outVerts, outTris, inOutBaseVertex, requiredLayer, onlyActive);
}

bool NavMeshGeometryExtractor::Extract(ModuleScene& scene, TriangleSoup& out, Layer requiredLayer, bool onlyActive)
{
    out.vertices.clear();
    out.indices.clear();

    int baseVertex = 0;
    for (GameObject* root : scene.getAllGameObjects())
        CollectFromObject(root, out.vertices, out.indices, baseVertex, requiredLayer, onlyActive);

    return !out.vertices.empty() && !out.indices.empty();
}