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



// Derives a deterministic string UID for a sub-asset embedded in a GLTF file.
static MD5Hash subAssetUID(const std::filesystem::path& gltfPath, const char* kind, int index)
{
    return computeMD5(gltfPath.string() + ":" + kind + ":" + std::to_string(index));
}

// Resolves a GLTF texture index to an engine asset UID, importing if needed.
static MD5Hash resolveTexture(const tinygltf::Model& model, int texIndex,
    const std::filesystem::path* modelPath)
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


void ImporterGltf::importTyped(const tinygltf::Model& source, PrefabAsset* prefab)
{
    std::vector<MD5Hash> matUIDs;
    matUIDs.reserve(source.materials.size());

    for (int i = 0; i < static_cast<int>(source.materials.size()); ++i)
    {
        const MD5Hash matUID = subAssetUID(*m_currentFilePath, "material", i);

        MaterialAsset mat(matUID);
        loadMaterial(source, source.materials[i], &mat);

        uint8_t* rawBuf = nullptr;
        const uint64_t bufSize = m_importerMaterial.save(&mat, &rawBuf);
        std::unique_ptr<uint8_t[]> buf(rawBuf);

        AssetMetadata meta;
        meta.uid = matUID;
        meta.type = AssetType::MATERIAL;
        app->getModuleAssets()->registerSubAsset(meta, buf.get(), static_cast<size_t>(bufSize));

        matUIDs.push_back(matUID);
    }

    std::vector<std::vector<MD5Hash>> meshPrimUIDs(source.meshes.size());
    int globalPrimIdx = 0;

    for (int mi = 0; mi < static_cast<int>(source.meshes.size()); ++mi)
    {
        const tinygltf::Mesh& gltfMesh = source.meshes[mi];
        meshPrimUIDs[mi].reserve(gltfMesh.primitives.size());

        for (int pi = 0; pi < static_cast<int>(gltfMesh.primitives.size()); ++pi, ++globalPrimIdx)
        {
            const MD5Hash meshUID = subAssetUID(*m_currentFilePath, "mesh", globalPrimIdx);

            // Resolve the material UID this primitive uses (may be INVALID)
            const tinygltf::Primitive& prim = gltfMesh.primitives[pi];
            const MD5Hash matUID = (prim.material >= 0 && prim.material < static_cast<int>(matUIDs.size()))
                ? matUIDs[prim.material] : INVALID_ASSET_ID;

            MeshAsset mesh(meshUID);
            loadMesh(source, prim, &mesh, matUID);

            uint8_t* rawBuf = nullptr;
            const uint64_t bufSize = m_importerMesh.save(&mesh, &rawBuf);
            std::unique_ptr<uint8_t[]> buf(rawBuf);

            AssetMetadata meta;
            meta.uid = meshUID;
            meta.type = AssetType::MESH;
            app->getModuleAssets()->registerSubAsset(meta, buf.get(), static_cast<size_t>(bufSize));

            meshPrimUIDs[mi].push_back(meshUID);
        }
    }

    // ---- 3. Build JSON hierarchy -------------------------------------------
    rapidjson::Document doc;
    doc.SetObject();

    rapidjson::Value gameObjectsArray(rapidjson::kArrayType);
    UID rootUID = 0;

    const int sceneIdx = source.defaultScene >= 0 ? source.defaultScene : 0;

    if (sceneIdx < static_cast<int>(source.scenes.size()))
    {
        bool firstRoot = true;
        for (int nodeIdx : source.scenes[sceneIdx].nodes)
        {
            UID uid = buildNodeJSON(source, nodeIdx, 0, meshPrimUIDs, matUIDs, gameObjectsArray, doc);
            if (firstRoot) { rootUID = uid; firstRoot = false; }
        }
    }

    const std::string rootUIDStr = std::to_string(rootUID);
    doc.AddMember("RootUID",
        rapidjson::Value(rootUIDStr.c_str(), doc.GetAllocator()),
        doc.GetAllocator());
    doc.AddMember("GameObjects", gameObjectsArray, doc.GetAllocator());

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(sb);
    doc.Accept(jsonWriter);

    prefab->m_json = sb.GetString();
    prefab->m_rootUID = rootUIDStr;

    m_currentFilePath = nullptr;
}


uint64_t ImporterGltf::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    const uint32_t jsonLen = static_cast<uint32_t>(src->m_json.size());

    uint64_t size = 0;
    size += sizeof(uint32_t) + src->m_uid.size();
    size += sizeof(uint32_t) + src->m_rootUID.size();
    size += sizeof(uint32_t) + jsonLen;

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter w(buffer);

    w.string(src->m_uid);
    w.string(src->m_rootUID);
    w.u32(jsonLen);
    w.bytes(src->m_json.data(), jsonLen);

    *outBuffer = buffer;
    return size;
}

void ImporterGltf::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    BinaryReader r(buffer);

    dst->m_uid = r.string();
    dst->m_rootUID = r.string();

    const uint32_t jsonLen = r.u32();
    dst->m_json.assign(reinterpret_cast<const char*>(r.ptr()), jsonLen);
}

UID ImporterGltf::buildNodeJSON(
    const tinygltf::Model& model,
    int nodeIndex,
    UID parentUID,
    const std::vector<std::vector<MD5Hash>>& meshPrimUIDs,
    const std::vector<MD5Hash>& matUIDs,
    rapidjson::Value& array,
    rapidjson::Document& doc)
{
    const tinygltf::Node& node = model.nodes[nodeIndex];
    auto& alloc = doc.GetAllocator();

    const UID goUID = hashToUID(subAssetUID(*m_currentFilePath, "node_go", nodeIndex));
    const UID xfUID = hashToUID(subAssetUID(*m_currentFilePath, "node_xform", nodeIndex));

    Vector3    pos = Vector3::Zero;
    Quaternion rot = Quaternion::Identity;
    Vector3    scale = Vector3::One;

    if (node.matrix.size() == 16)
    {
        // GLTF stores matrices column-major; DirectX SimpleMath uses row-major.
        Matrix m(
            (float)node.matrix[0], (float)node.matrix[4], (float)node.matrix[8], (float)node.matrix[12],
            (float)node.matrix[1], (float)node.matrix[5], (float)node.matrix[9], (float)node.matrix[13],
            (float)node.matrix[2], (float)node.matrix[6], (float)node.matrix[10], (float)node.matrix[14],
            (float)node.matrix[3], (float)node.matrix[7], (float)node.matrix[11], (float)node.matrix[15]
        );
        m.Decompose(scale, rot, pos);
    }
    else
    {
        if (node.translation.size() == 3)
            pos = { (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] };
        if (node.rotation.size() == 4)
            rot = { (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3] };
        if (node.scale.size() == 3)
            scale = { (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] };
    }

    // --- Primitive info for this node's mesh --------------------------------
    const int meshIdx = node.mesh;
    const bool hasMesh = meshIdx >= 0 && meshIdx < static_cast<int>(meshPrimUIDs.size());
    const int primCount = hasMesh ? static_cast<int>(meshPrimUIDs[meshIdx].size()) : 0;

    // ---- Build the node's own GameObject entry ----------------------------
    const std::string nodeName = node.name.empty() ? ("Node_" + std::to_string(nodeIndex)) : node.name;

    rapidjson::Value go(rapidjson::kObjectType);
    go.AddMember("UID", goUID, alloc);
    go.AddMember("ParentUID", parentUID, alloc);
    go.AddMember("Name", rapidjson::Value(nodeName.c_str(), alloc), alloc);
    go.AddMember("Active", true, alloc);
    go.AddMember("Static", false, alloc);
    go.AddMember("Layer", rapidjson::Value("DEFAULT", alloc), alloc);
    go.AddMember("Tag", rapidjson::Value("DEFAULT", alloc), alloc);

    // Transform
    {
        rapidjson::Value xform(rapidjson::kObjectType);
        xform.AddMember("UID", xfUID, alloc);

        rapidjson::Value posArr(rapidjson::kArrayType);
        posArr.PushBack(pos.x, alloc); posArr.PushBack(pos.y, alloc); posArr.PushBack(pos.z, alloc);
        xform.AddMember("Position", posArr, alloc);

        rapidjson::Value rotArr(rapidjson::kArrayType);
        rotArr.PushBack(rot.x, alloc); rotArr.PushBack(rot.y, alloc);
        rotArr.PushBack(rot.z, alloc); rotArr.PushBack(rot.w, alloc);
        xform.AddMember("Rotation", rotArr, alloc);

        rapidjson::Value scaleArr(rapidjson::kArrayType);
        scaleArr.PushBack(scale.x, alloc); scaleArr.PushBack(scale.y, alloc); scaleArr.PushBack(scale.z, alloc);
        xform.AddMember("Scale", scaleArr, alloc);

        go.AddMember("Transform", xform, alloc);
    }

    // Components: attach a MeshRenderer if this node has exactly one primitive
    {
        rapidjson::Value comps(rapidjson::kArrayType);

        if (primCount == 1)
        {
            const tinygltf::Primitive& prim = model.meshes[meshIdx].primitives[0];
            const MD5Hash& meshUID = meshPrimUIDs[meshIdx][0];
            const MD5Hash  matUID = (prim.material >= 0 && prim.material < static_cast<int>(matUIDs.size()))
                ? matUIDs[prim.material] : INVALID_ASSET_ID;
            const UID rendUID = hashToUID(subAssetUID(*m_currentFilePath, "node_rend", nodeIndex));

            rapidjson::Value comp(rapidjson::kObjectType);
            comp.AddMember("UID", rendUID, alloc);
            comp.AddMember("ComponentType", static_cast<int>(ComponentType::MODEL), alloc);
            comp.AddMember("Active", true, alloc);
            comp.AddMember("MeshAssetId", rapidjson::Value(meshUID.c_str(), alloc), alloc);
            comp.AddMember("MaterialAssetId", rapidjson::Value(matUID.c_str(), alloc), alloc);
            comps.PushBack(comp, alloc);
        }

        go.AddMember("Components", comps, alloc);
    }

    array.PushBack(go, alloc);

    // ---- Multi-primitive: one child GameObject per primitive ---------------
    if (primCount > 1)
    {
        const tinygltf::Mesh& gltfMesh = model.meshes[meshIdx];
        const std::string meshName = gltfMesh.name.empty()
            ? ("Mesh_" + std::to_string(meshIdx)) : gltfMesh.name;

        for (int pi = 0; pi < primCount; ++pi)
        {
            const int key = nodeIndex * 1000 + pi;
            const UID primGoUID = hashToUID(subAssetUID(*m_currentFilePath, "prim_go", key));
            const UID primXfUID = hashToUID(subAssetUID(*m_currentFilePath, "prim_xform", key));
            const UID primRnUID = hashToUID(subAssetUID(*m_currentFilePath, "prim_rend", key));

            const tinygltf::Primitive& prim = gltfMesh.primitives[pi];
            const MD5Hash& meshUID = meshPrimUIDs[meshIdx][pi];
            const MD5Hash  matUID = (prim.material >= 0 && prim.material < static_cast<int>(matUIDs.size()))
                ? matUIDs[prim.material] : INVALID_ASSET_ID;

            const std::string primName = meshName + "_Prim" + std::to_string(pi);

            rapidjson::Value primGo(rapidjson::kObjectType);
            primGo.AddMember("UID", primGoUID, alloc);
            primGo.AddMember("ParentUID", goUID, alloc);
            primGo.AddMember("Name", rapidjson::Value(primName.c_str(), alloc), alloc);
            primGo.AddMember("Active", true, alloc);
            primGo.AddMember("Static", false, alloc);
            primGo.AddMember("Layer", rapidjson::Value("DEFAULT", alloc), alloc);
            primGo.AddMember("Tag", rapidjson::Value("DEFAULT", alloc), alloc);

            // Identity transform — inherits parent's world position
            {
                rapidjson::Value xform(rapidjson::kObjectType);
                xform.AddMember("UID", primXfUID, alloc);
                rapidjson::Value pArr(rapidjson::kArrayType);
                pArr.PushBack(0.f, alloc); pArr.PushBack(0.f, alloc); pArr.PushBack(0.f, alloc);
                xform.AddMember("Position", pArr, alloc);
                rapidjson::Value rArr(rapidjson::kArrayType);
                rArr.PushBack(0.f, alloc); rArr.PushBack(0.f, alloc);
                rArr.PushBack(0.f, alloc); rArr.PushBack(1.f, alloc);
                xform.AddMember("Rotation", rArr, alloc);
                rapidjson::Value sArr(rapidjson::kArrayType);
                sArr.PushBack(1.f, alloc); sArr.PushBack(1.f, alloc); sArr.PushBack(1.f, alloc);
                xform.AddMember("Scale", sArr, alloc);
                primGo.AddMember("Transform", xform, alloc);
            }

            // MeshRenderer component
            {
                rapidjson::Value comps(rapidjson::kArrayType);
                rapidjson::Value comp(rapidjson::kObjectType);
                comp.AddMember("UID", primRnUID, alloc);
                comp.AddMember("ComponentType", static_cast<int>(ComponentType::MODEL), alloc);
                comp.AddMember("Active", true, alloc);
                comp.AddMember("MeshAssetId", rapidjson::Value(meshUID.c_str(), alloc), alloc);
                comp.AddMember("MaterialAssetId", rapidjson::Value(matUID.c_str(), alloc), alloc);
                comps.PushBack(comp, alloc);
                primGo.AddMember("Components", comps, alloc);
            }

            array.PushBack(primGo, alloc);
        }
    }

    // ---- Recurse into GLTF child nodes ------------------------------------
    for (int childIdx : node.children)
        buildNodeJSON(model, childIdx, goUID, meshPrimUIDs, matUIDs, array, doc);

    return goUID;
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

void ImporterGltf::loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material,
    MaterialAsset* mat)
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