#include "Globals.h"
#include "ImporterGltf.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4018)
#pragma warning(disable : 4267)
#include "tiny_gltf.h"
#pragma warning(pop)

#include "PrefabAsset.h"
#include "MeshAsset.h"
#include "MaterialAsset.h"
#include "ComponentType.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleFileSystem.h"

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

static const DXGI_FORMAT INDEX_FORMATS[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };


// Resolves a GLTF texture index to an engine asset UID, importing if needed.
MD5Hash resolveTexture(const tinygltf::Model& model, int texIndex,const std::filesystem::path* modelPath)
{
    if (texIndex < 0 || texIndex >= static_cast<int>(model.textures.size()))
        return INVALID_ASSET_ID;

    const tinygltf::Texture& tex = model.textures[texIndex];
    if (tex.source < 0 || tex.source >= static_cast<int>(model.images.size()))
        return INVALID_ASSET_ID;

    const tinygltf::Image& image = model.images[tex.source];
    if (image.uri.empty())
        return INVALID_ASSET_ID;

    std::filesystem::path resolved = modelPath->parent_path() / image.uri;
    MD5Hash uid = app->getModuleAssets()->findUID(resolved);
    if (!isValidAsset(uid))
        app->getModuleAssets()->importAsset(resolved, uid);

    return uid;
}


ImporterGltf::ImporterGltf(ImporterMesh& importerMesh, ImporterMaterial& importerMaterial): m_importerMesh(importerMesh), m_importerMaterial(importerMaterial)
{
}

bool ImporterGltf::canImport(const std::filesystem::path& path) const
{
    return path.extension().string() == GLTF_EXTENSION;
}

Asset* ImporterGltf::createAssetInstance(const MD5Hash& uid) const
{
    return new PrefabAsset(uid);
}

bool ImporterGltf::loadExternal(const std::filesystem::path& path, tinygltf::Model& out)
{
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    const std::string pathStr = path.string();
    if (!gltfContext.LoadASCIIFromFile(&out, &error, &warning, pathStr.c_str()))
    {
        DEBUG_ERROR("[ImporterGltf] Failed to load GLTF from '%s': %s", pathStr.c_str(), error.c_str());
        return false;
    }

    m_currentFilePath = &path;
    return true;
}


void ImporterGltf::importTyped(const tinygltf::Model& model, PrefabAsset* dst)
{
    ModuleAssets* assets = app->getModuleAssets();

    // 1. Import materials
    std::vector<MD5Hash> materialUIDs(model.materials.size(), INVALID_ASSET_ID);
    for (int i = 0; i < static_cast<int>(model.materials.size()); ++i)
    {
        // Derive a stable UID from the source file + material index
        const std::string matKey = m_currentFilePath->string() + "?mat=" + std::to_string(i);
        MD5Hash matUID = computeMD5(matKey);

        MaterialAsset matAsset(matUID);
        loadMaterial(model, model.materials[i], &matAsset);

        uint8_t* rawBuffer = nullptr;
        const uint64_t size = m_importerMaterial.save(&matAsset, &rawBuffer);
        std::unique_ptr<uint8_t[]> buf(rawBuffer);

        AssetMetadata meta;
        meta.uid = matUID;
        meta.type = AssetType::MATERIAL;
        assets->registerSubAsset(meta, rawBuffer, static_cast<size_t>(size));

        materialUIDs[i] = matUID;
    }

    // 2. Import meshes (one MeshAsset per GLTF mesh, merging all primitives)
    std::vector<MD5Hash> meshUIDs(model.meshes.size(), INVALID_ASSET_ID);
    for (int i = 0; i < static_cast<int>(model.meshes.size()); ++i)
    {
        const std::string meshKey = m_currentFilePath->string() + "?mesh=" + std::to_string(i);
        MD5Hash meshUID = computeMD5(meshKey);

        MeshAsset meshAsset(meshUID);
        for (const tinygltf::Primitive& prim : model.meshes[i].primitives)
        {
            MD5Hash matUID = (prim.material >= 0 && prim.material < static_cast<int>(materialUIDs.size()))
                ? materialUIDs[prim.material]
                : INVALID_ASSET_ID;
            loadMesh(model, prim, &meshAsset, matUID);
        }

        uint8_t* rawBuffer = nullptr;
        const uint64_t size = m_importerMesh.save(&meshAsset, &rawBuffer);
        std::unique_ptr<uint8_t[]> buf(rawBuffer);

        AssetMetadata meta;
        meta.uid = meshUID;
        meta.type = AssetType::MESH;
        assets->registerSubAsset(meta, rawBuffer, static_cast<size_t>(size));

        meshUIDs[i] = meshUID;
    }

    //3. Build prefab JSON from GLTF node hierarchy
    using namespace rapidjson;
    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    // Pick a stable prefab name from the source file stem
    const std::string prefabName = m_currentFilePath->stem().string();
    const MD5Hash     prefabUID = dst->m_uid;

    doc.AddMember("PrefabName", Value(prefabName.c_str(), alloc), alloc);
    doc.AddMember("Version", 2, alloc);
    doc.AddMember("PrefabUID", Value(prefabUID.c_str(), alloc), alloc);

    // Recursive lambda: GLTF node → prefab JSON node
    std::function<Value(int)> buildNode = [&](int nodeIndex) -> Value
        {
            const tinygltf::Node& gltfNode = model.nodes[nodeIndex];

            Value nodeObj(kObjectType);
            const std::string nodeName = gltfNode.name.empty()
                ? ("Node_" + std::to_string(nodeIndex))
                : gltfNode.name;
            nodeObj.AddMember("Name", Value(nodeName.c_str(), alloc), alloc);
            nodeObj.AddMember("Active", true, alloc);

            // Transform
            Value tfObj(kObjectType);
            if (gltfNode.translation.size() == 3)
            {
                Value pos(kArrayType);
                pos.PushBack((float)gltfNode.translation[0], alloc)
                    .PushBack((float)gltfNode.translation[1], alloc)
                    .PushBack((float)gltfNode.translation[2], alloc);
                tfObj.AddMember("position", pos, alloc);
            }
            else
            {
                Value pos(kArrayType);
                pos.PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(0.f, alloc);
                tfObj.AddMember("position", pos, alloc);
            }
            if (gltfNode.rotation.size() == 4)
            {
                Value rot(kArrayType);
                rot.PushBack((float)gltfNode.rotation[0], alloc)
                    .PushBack((float)gltfNode.rotation[1], alloc)
                    .PushBack((float)gltfNode.rotation[2], alloc)
                    .PushBack((float)gltfNode.rotation[3], alloc);
                tfObj.AddMember("rotation", rot, alloc);
            }
            else
            {
                Value rot(kArrayType);
                rot.PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(1.f, alloc);
                tfObj.AddMember("rotation", rot, alloc);
            }
            if (gltfNode.scale.size() == 3)
            {
                Value sc(kArrayType);
                sc.PushBack((float)gltfNode.scale[0], alloc)
                    .PushBack((float)gltfNode.scale[1], alloc)
                    .PushBack((float)gltfNode.scale[2], alloc);
                tfObj.AddMember("scale", sc, alloc);
            }
            else
            {
                Value sc(kArrayType);
                sc.PushBack(1.f, alloc).PushBack(1.f, alloc).PushBack(1.f, alloc);
                tfObj.AddMember("scale", sc, alloc);
            }
            nodeObj.AddMember("Transform", tfObj, alloc);

            // Components — emit MeshRenderer if this node has a mesh
            Value compArray(kArrayType);
            if (gltfNode.mesh >= 0 && gltfNode.mesh < static_cast<int>(meshUIDs.size()))
            {
                const MD5Hash& meshUID = meshUIDs[gltfNode.mesh];

                // Pick the first primitive's material as the node-level material
                MD5Hash matUID = INVALID_ASSET_ID;
                if (!model.meshes[gltfNode.mesh].primitives.empty())
                {
                    int matIdx = model.meshes[gltfNode.mesh].primitives[0].material;
                    if (matIdx >= 0 && matIdx < static_cast<int>(materialUIDs.size()))
                        matUID = materialUIDs[matIdx];
                }

                Value compData(kObjectType);
                compData.AddMember("MeshAssetId", Value(meshUID.c_str(), alloc), alloc);
                compData.AddMember("MaterialAssetId", Value(matUID.c_str(), alloc), alloc);

                Value compNode(kObjectType);
                compNode.AddMember("Type", static_cast<int>(ComponentType::MODEL), alloc);
                compNode.AddMember("Data", compData, alloc);

                compArray.PushBack(compNode, alloc);
            }
            nodeObj.AddMember("Components", compArray, alloc);

            // Children
            Value childArray(kArrayType);
            for (int childIdx : gltfNode.children)
            {
                childArray.PushBack(buildNode(childIdx), alloc);
            }

            nodeObj.AddMember("Children", childArray, alloc);

            return nodeObj;
        };

    // Find root nodes from the default scene (or all nodes if no scenes)
    std::vector<int> rootNodes;
    if (!model.scenes.empty())
    {
        int sceneIdx = model.defaultScene >= 0 ? model.defaultScene : 0;
        rootNodes = model.scenes[sceneIdx].nodes;
    }
    else
    {
        for (int i = 0; i < static_cast<int>(model.nodes.size()); ++i)
            rootNodes.push_back(i);
    }

    // Wrap in a single synthetic root if there are multiple top-level nodes
    Value gameObjectNode;
    if (rootNodes.size() == 1)
    {
        gameObjectNode = buildNode(rootNodes[0]);
    }
    else
    {
        gameObjectNode.SetObject();
        gameObjectNode.AddMember("Name", Value(prefabName.c_str(), alloc), alloc);
        gameObjectNode.AddMember("Active", true, alloc);

        Value tfObj(kObjectType);
        Value pos(kArrayType); pos.PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(0.f, alloc);
        Value rot(kArrayType); rot.PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(1.f, alloc);
        Value sc(kArrayType);  sc.PushBack(1.f, alloc).PushBack(1.f, alloc).PushBack(1.f, alloc);
        tfObj.AddMember("position", pos, alloc);
        tfObj.AddMember("rotation", rot, alloc);
        tfObj.AddMember("scale", sc, alloc);
        gameObjectNode.AddMember("Transform", tfObj, alloc);
        gameObjectNode.AddMember("Components", Value(kArrayType), alloc);

        Value childArray(kArrayType);
        for (int rootIdx : rootNodes)
            childArray.PushBack(buildNode(rootIdx), alloc);
        gameObjectNode.AddMember("Children", childArray, alloc);
    }

    doc.AddMember("GameObject", gameObjectNode, alloc);

    StringBuffer sb;
    rapidjson::Writer<StringBuffer> writer(sb);
    doc.Accept(writer);

    dst->m_json = sb.GetString();
    dst->m_rootUID = prefabUID;

    m_currentFilePath = nullptr;   // reset for safety
}

uint64_t ImporterGltf::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    // Reuse the same binary layout as ImporterPrefab
    const uint32_t jsonLen = static_cast<uint32_t>(src->m_json.size());
    const uint32_t rootLen = static_cast<uint32_t>(src->m_rootUID.size());
    const uint64_t totalSize = sizeof(uint32_t) + jsonLen + sizeof(uint32_t) + rootLen;

    uint8_t* buffer = new uint8_t[totalSize];
    BinaryWriter writer(buffer);
    writer.string(src->m_json);
    writer.string(src->m_rootUID);

    *outBuffer = buffer;
    return totalSize;
}

void ImporterGltf::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    BinaryReader reader(buffer);
    dst->m_json = reader.string();
    dst->m_rootUID = reader.string();
}

void ImporterGltf::loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
    MeshAsset* mesh, const MD5Hash& materialUID)
{
    const uint32_t baseVertex = static_cast<uint32_t>(mesh->vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(mesh->indices.size());

    auto itPos = primitive.attributes.find("POSITION");
    if (itPos == primitive.attributes.end()) return;

    const tinygltf::Accessor& posAccessor = model.accessors[itPos->second];
    const uint32_t vertexCount = static_cast<uint32_t>(posAccessor.count);

    mesh->vertices.resize(baseVertex + vertexCount);
    uint8_t* vBase = reinterpret_cast<uint8_t*>(mesh->vertices.data() + baseVertex);

    loadAccessorData(vBase + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), vertexCount, model, itPos->second);
    loadAccessorData(vBase + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), vertexCount, model, primitive.attributes, "NORMAL");
    loadAccessorData(vBase + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), vertexCount, model, primitive.attributes, "TEXCOORD_0");

    // Indices
    uint32_t indexCount = 0;
    uint32_t componentSize = 0;

    if (primitive.indices >= 0)
    {
        const tinygltf::Accessor& idxAccessor = model.accessors[primitive.indices];
        const int ct = idxAccessor.componentType;

        if (ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE ||
            ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
            ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
        {
            componentSize = tinygltf::GetComponentSizeInBytes(ct);
            indexCount = static_cast<uint32_t>(idxAccessor.count);

            const uint32_t prevSize = static_cast<uint32_t>(mesh->indices.size());
            mesh->indices.resize(prevSize + indexCount * componentSize);
            loadAccessorData(mesh->indices.data() + prevSize, componentSize, componentSize, indexCount, model, primitive.indices);

            mesh->indexFormat = INDEX_FORMATS[componentSize >> 1];
        }
    }
    else
    {
        // No index buffer — generate a trivial sequential index list
        componentSize = sizeof(uint32_t);
        indexCount = vertexCount;
        const uint32_t prevSize = static_cast<uint32_t>(mesh->indices.size());
        mesh->indices.resize(prevSize + indexCount * componentSize);
        uint32_t* dst = reinterpret_cast<uint32_t*>(mesh->indices.data() + prevSize);
        for (uint32_t i = 0; i < vertexCount; ++i)
            dst[i] = baseVertex + i;
        mesh->indexFormat = DXGI_FORMAT_R32_UINT;
    }

    Submesh submesh{};
    submesh.indexStart = baseIndex / std::max(componentSize, 1u);
    submesh.indexCount = indexCount;
    submesh.materialId = materialUID;
    mesh->submeshes.push_back(submesh);

    // Bounds from accessor min/max
    if (posAccessor.minValues.size() == 3 && posAccessor.maxValues.size() == 3)
    {
        const Vector3 bMin((float)posAccessor.minValues[0], (float)posAccessor.minValues[1], (float)posAccessor.minValues[2]);
        const Vector3 bMax((float)posAccessor.maxValues[0], (float)posAccessor.maxValues[1], (float)posAccessor.maxValues[2]);
        mesh->boundsCenter = (bMin + bMax) * 0.5f;
        mesh->boundsExtents = (bMax - bMin) * 0.5f;
    }
}

void ImporterGltf::loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* mat)
{
    const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;

    mat->baseColour = Color(
        float(pbr.baseColorFactor[0]),
        float(pbr.baseColorFactor[1]),
        float(pbr.baseColorFactor[2]),
        float(pbr.baseColorFactor[3]));

    mat->metallicFactor = static_cast<uint32_t>(pbr.metallicFactor * 255.0f);

    mat->baseMap = resolveTexture(model, pbr.baseColorTexture.index, m_currentFilePath);
    mat->metallicRoughnessMap = resolveTexture(model, pbr.metallicRoughnessTexture.index, m_currentFilePath);
    mat->normalMap = resolveTexture(model, material.normalTexture.index, m_currentFilePath);
    mat->occlusionMap = resolveTexture(model, material.occlusionTexture.index, m_currentFilePath);
    mat->emissiveMap = resolveTexture(model, material.emissiveTexture.index, m_currentFilePath);
    mat->isEmissive = isValidAsset(mat->emissiveMap);
}