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
#include "MeshRenderer.h"
#include "GameObject.h"
#include "Transform.h"
#include "PrefabManager.h"

#include "Application.h"
#include "ModuleAssets.h"

#include <functional>

static const DXGI_FORMAT INDEX_FORMATS[3] = {
    DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };

static MD5Hash resolveTexture(const tinygltf::Model& model, int texIndex,
    const std::filesystem::path* modelPath)
{
    if (texIndex < 0 || texIndex >= static_cast<int>(model.textures.size()))
        return INVALID_ASSET_ID;
    const tinygltf::Texture& tex = model.textures[texIndex];
    if (tex.source < 0 || tex.source >= static_cast<int>(model.images.size()))
        return INVALID_ASSET_ID;
    const tinygltf::Image& img = model.images[tex.source];
    if (img.uri.empty()) return INVALID_ASSET_ID;
    std::filesystem::path resolved = modelPath->parent_path() / img.uri;
    MD5Hash uid = app->getModuleAssets()->findUID(resolved);
    if (!isValidAsset(uid)) app->getModuleAssets()->importAsset(resolved, uid);
    return uid;
}

ImporterGltf::ImporterGltf(ImporterMesh& importerMesh,
    ImporterMaterial& importerMaterial)
    : m_importerMesh(importerMesh)
    , m_importerMaterial(importerMaterial)
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
    tinygltf::TinyGLTF ctx;
    std::string err, warn;
    if (!ctx.LoadASCIIFromFile(&out, &err, &warn, path.string().c_str()))
    {
        DEBUG_ERROR("[ImporterGltf] Failed to load '%s': %s",
            path.string().c_str(), err.c_str());
        return false;
    }
    m_currentFilePath = &path;
    return true;
}

// ---------------------------------------------------------------------------
// importTyped — three phases:
//
//   Phase 1  Register MaterialAsset and MeshAsset sub-assets.
//   Phase 2  Build a temporary GameObject tree (no ModuleScene dependency).
//            MeshRenderer asset UIDs are set directly — no GPU allocation.
//   Phase 3  Fill all PrefabData fields:
//              - m_json        via PrefabManager::buildPrefabJSON
//              - m_sourcePath  = the .gltf file path (full path, not derived)
//              - m_name        = path stem
//              - m_assetUID    = asset system MD5
//              - m_prefabUID   = root->GetID() — the root GO's engine UID
// ---------------------------------------------------------------------------
void ImporterGltf::importTyped(const tinygltf::Model& model, PrefabAsset* dst)
{
    ModuleAssets* assets = app->getModuleAssets();

    // ── Phase 1a: materials ──────────────────────────────────────────────────
    std::vector<MD5Hash> materialUIDs(model.materials.size(), INVALID_ASSET_ID);
    for (int i = 0; i < static_cast<int>(model.materials.size()); ++i)
    {
        const MD5Hash matUID =
            computeMD5(m_currentFilePath->string() + "?mat=" + std::to_string(i));
        MaterialAsset matAsset(matUID);
        loadMaterial(model, model.materials[i], &matAsset);

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerMaterial.save(&matAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        AssetMetadata meta; meta.uid = matUID; meta.type = AssetType::MATERIAL;
        assets->registerSubAsset(meta, rawBuf, static_cast<size_t>(size));
        materialUIDs[i] = matUID;
    }

    // ── Phase 1b: meshes ─────────────────────────────────────────────────────
    std::vector<MD5Hash> meshUIDs(model.meshes.size(), INVALID_ASSET_ID);
    for (int i = 0; i < static_cast<int>(model.meshes.size()); ++i)
    {
        const MD5Hash meshUID =
            computeMD5(m_currentFilePath->string() + "?mesh=" + std::to_string(i));
        MeshAsset meshAsset(meshUID);
        for (const tinygltf::Primitive& prim : model.meshes[i].primitives)
        {
            const MD5Hash matUID =
                (prim.material >= 0 && prim.material < static_cast<int>(materialUIDs.size()))
                ? materialUIDs[prim.material] : INVALID_ASSET_ID;
            loadMesh(model, prim, &meshAsset, matUID);
        }

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerMesh.save(&meshAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        AssetMetadata meta; meta.uid = meshUID; meta.type = AssetType::MESH;
        assets->registerSubAsset(meta, rawBuf, static_cast<size_t>(size));
        meshUIDs[i] = meshUID;
    }

    // ── Phase 2: temporary GameObject tree ───────────────────────────────────
    std::vector<std::unique_ptr<GameObject>> tempObjects;

    auto makeNode = [&](const std::string& name) -> GameObject*
        {
            auto go = std::make_unique<GameObject>(GenerateUID(), GenerateUID());
            go->SetName(name);
            GameObject* raw = go.get();
            tempObjects.push_back(std::move(go));
            return raw;
        };

    std::function<GameObject* (int, GameObject*)> buildNode =
        [&](int nodeIdx, GameObject* parent) -> GameObject*
        {
            const tinygltf::Node& gNode = model.nodes[nodeIdx];
            const std::string name = gNode.name.empty()
                ? ("Node_" + std::to_string(nodeIdx)) : gNode.name;

            GameObject* go = makeNode(name);
            Transform* tf = go->GetTransform();

            if (gNode.translation.size() == 3)
                tf->setPosition(Vector3((float)gNode.translation[0],
                    (float)gNode.translation[1],
                    (float)gNode.translation[2]));
            if (gNode.rotation.size() == 4)
                tf->setRotation(Quaternion((float)gNode.rotation[0],
                    (float)gNode.rotation[1],
                    (float)gNode.rotation[2],
                    (float)gNode.rotation[3]));
            if (gNode.scale.size() == 3)
                tf->setScale(Vector3((float)gNode.scale[0],
                    (float)gNode.scale[1],
                    (float)gNode.scale[2]));

            if (parent)
            {
                Transform* parentTf = parent->GetTransform();
                tf->setRoot(parentTf);
                parentTf->addChild(go);
            }

            if (gNode.mesh >= 0 && gNode.mesh < static_cast<int>(meshUIDs.size()))
            {
                auto* mr = static_cast<MeshRenderer*>(
                    go->AddComponentWithUID(ComponentType::MODEL, GenerateUID()));
                if (mr)
                {
                    mr->getMeshReference() = meshUIDs[gNode.mesh];
                    const auto& prims = model.meshes[gNode.mesh].primitives;
                    if (!prims.empty() && prims[0].material >= 0
                        && prims[0].material < static_cast<int>(materialUIDs.size()))
                        mr->getMaterialReference() = materialUIDs[prims[0].material];
                }
            }

            for (int childIdx : gNode.children)
                buildNode(childIdx, go);
            return go;
        };

    std::vector<int> rootNodes;
    if (!model.scenes.empty())
    {
        const int sceneIdx = model.defaultScene >= 0 ? model.defaultScene : 0;
        rootNodes = model.scenes[sceneIdx].nodes;
    }
    else
    {
        rootNodes.reserve(model.nodes.size());
        for (int i = 0; i < static_cast<int>(model.nodes.size()); ++i)
            rootNodes.push_back(i);
    }

    const std::string prefabName = m_currentFilePath->stem().string();
    GameObject* root = nullptr;
    if (rootNodes.size() == 1)
        root = buildNode(rootNodes[0], nullptr);
    else
    {
        root = makeNode(prefabName);
        for (int idx : rootNodes)
            buildNode(idx, root);
    }

    // ── Phase 3: fill all PrefabData fields ───────────────────────────────────
    // savePath = the .gltf source file path — full path, no hardcoded folder.
    // m_prefabUID = root->GetID() — the root GO's uint64_t engine UID,
    //              NOT a 32-bit name hash.
    PrefabData& data = dst->getData();
    data.m_json = PrefabManager::buildPrefabJSON(root, *m_currentFilePath);
    data.m_sourcePath = *m_currentFilePath;
    data.m_name = prefabName;
    data.m_assetUID = dst->m_uid;
    data.m_prefabUID = root->GetID();   // GO UID — the only UID that matters
    // m_overrides stays default-constructed (empty).

    m_currentFilePath = nullptr;
    // tempObjects destroyed here — all temporary GameObjects are freed.
}

// ──────────────────────────────────────────────────────────────────────────────
// Binary cache — same layout as ImporterPrefab for uniform load<PrefabAsset>.
// [string m_sourcePath][string m_name][string m_assetUID][uint64 m_prefabUID][string m_json]
// ──────────────────────────────────────────────────────────────────────────────
uint64_t ImporterGltf::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    const PrefabData& data = src->getData();
    const std::string pathStr = data.m_sourcePath.string();

    uint64_t size = 0;
    size += sizeof(uint32_t) + pathStr.size();
    size += sizeof(uint32_t) + data.m_name.size();
    size += sizeof(uint32_t) + data.m_assetUID.size();
    size += sizeof(uint64_t);
    size += sizeof(uint32_t) + data.m_json.size();

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);
    writer.string(pathStr);
    writer.string(data.m_name);
    writer.string(data.m_assetUID);
    writer.u64(static_cast<uint64_t>(data.m_prefabUID));
    writer.string(data.m_json);

    *outBuffer = buffer;
    return size;
}

void ImporterGltf::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    PrefabData& data = dst->getData();
    BinaryReader reader(buffer);
    data.m_sourcePath = reader.string();
    data.m_name = reader.string();
    data.m_assetUID = reader.string();
    data.m_prefabUID = static_cast<UID>(reader.u64());
    data.m_json = reader.string();
}

// ---------------------------------------------------------------------------
// Sub-object loaders
// ---------------------------------------------------------------------------
void ImporterGltf::loadMesh(const tinygltf::Model& model,
    const tinygltf::Primitive& primitive,
    MeshAsset* mesh, const MD5Hash& materialUID)
{
    const uint32_t baseVertex = static_cast<uint32_t>(mesh->vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(mesh->indices.size());

    auto itPos = primitive.attributes.find("POSITION");
    if (itPos == primitive.attributes.end()) return;

    const tinygltf::Accessor& posAcc = model.accessors[itPos->second];
    const uint32_t vertexCount = static_cast<uint32_t>(posAcc.count);

    mesh->vertices.resize(baseVertex + vertexCount);
    uint8_t* vBase = reinterpret_cast<uint8_t*>(mesh->vertices.data() + baseVertex);

    loadAccessorData(vBase + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex),
        vertexCount, model, itPos->second);
    loadAccessorData(vBase + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex),
        vertexCount, model, primitive.attributes, "NORMAL");
    loadAccessorData(vBase + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex),
        vertexCount, model, primitive.attributes, "TEXCOORD_0");

    uint32_t indexCount = 0, componentSize = 0;
    if (primitive.indices >= 0)
    {
        const tinygltf::Accessor& idxAcc = model.accessors[primitive.indices];
        const int ct = idxAcc.componentType;
        if (ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE ||
            ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
            ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
        {
            componentSize = tinygltf::GetComponentSizeInBytes(ct);
            indexCount = static_cast<uint32_t>(idxAcc.count);
            const uint32_t prev = static_cast<uint32_t>(mesh->indices.size());
            mesh->indices.resize(prev + indexCount * componentSize);
            loadAccessorData(mesh->indices.data() + prev, componentSize, componentSize,
                indexCount, model, primitive.indices);
            mesh->indexFormat = INDEX_FORMATS[componentSize >> 1];
        }
    }
    else
    {
        componentSize = sizeof(uint32_t);
        indexCount = vertexCount;
        const uint32_t prev = static_cast<uint32_t>(mesh->indices.size());
        mesh->indices.resize(prev + indexCount * componentSize);
        uint32_t* dst = reinterpret_cast<uint32_t*>(mesh->indices.data() + prev);
        for (uint32_t i = 0; i < vertexCount; ++i) dst[i] = baseVertex + i;
        mesh->indexFormat = DXGI_FORMAT_R32_UINT;
    }

    Submesh submesh{};
    submesh.indexStart = baseIndex / std::max(componentSize, 1u);
    submesh.indexCount = indexCount;
    submesh.materialId = materialUID;
    mesh->submeshes.push_back(submesh);

    if (posAcc.minValues.size() == 3 && posAcc.maxValues.size() == 3)
    {
        const Vector3 bMin((float)posAcc.minValues[0],
            (float)posAcc.minValues[1],
            (float)posAcc.minValues[2]);
        const Vector3 bMax((float)posAcc.maxValues[0],
            (float)posAcc.maxValues[1],
            (float)posAcc.maxValues[2]);
        mesh->boundsCenter = (bMin + bMax) * 0.5f;
        mesh->boundsExtents = (bMax - bMin) * 0.5f;
    }
}

void ImporterGltf::loadMaterial(const tinygltf::Model& model,
    const tinygltf::Material& material,
    MaterialAsset* mat)
{
    const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;
    mat->baseColour = Color(
        float(pbr.baseColorFactor[0]), float(pbr.baseColorFactor[1]),
        float(pbr.baseColorFactor[2]), float(pbr.baseColorFactor[3]));
    mat->metallicFactor = static_cast<uint32_t>(pbr.metallicFactor * 255.0f);
    mat->baseMap = resolveTexture(model, pbr.baseColorTexture.index, m_currentFilePath);
    mat->metallicRoughnessMap = resolveTexture(model, pbr.metallicRoughnessTexture.index, m_currentFilePath);
    mat->normalMap = resolveTexture(model, material.normalTexture.index, m_currentFilePath);
    mat->occlusionMap = resolveTexture(model, material.occlusionTexture.index, m_currentFilePath);
    mat->emissiveMap = resolveTexture(model, material.emissiveTexture.index, m_currentFilePath);
    mat->isEmissive = isValidAsset(mat->emissiveMap);
}