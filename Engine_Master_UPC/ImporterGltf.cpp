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

#include "AnimationStateMachineAsset.h"
#include "SkinAsset.h"
#include "AnimationAsset.h"
#include "PrefabAsset.h"
#include "MeshAsset.h"
#include "MaterialAsset.h"
#include "ComponentType.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include "Transform.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "MD5.h"

#include "ImporterMesh.h"
#include "ImporterMaterial.h"
#include "ImporterPrefab.h"
#include "ImporterAnimation.h"
#include "ImporterSkin.h"
#include "ImporterAnimationStateMachine.h"
#include "Metadata.h"

#include <functional>
#include <algorithm>
#include <cctype>
#include <array>

static const DXGI_FORMAT INDEX_FORMATS[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };

static float getVec4Component(const Vector4& v, int index)
{
    switch (index)
    {
    case 0: return v.x;
    case 1: return v.y;
    case 2: return v.z;
    default: return v.w;
    }
}

static void setVec4Component(Vector4& v, int index, float value)
{
    switch (index)
    {
    case 0: v.x = value; break;
    case 1: v.y = value; break;
    case 2: v.z = value; break;
    default: v.w = value; break;
    }
}

struct JointInfluence
{
    uint16_t joint = 0;
    float weight = 0.0f;
};

static void collapseToTop4Influences(
    const uint16_t joints0[4],
    const Vector4& weights0,
    const uint16_t joints1[4],
    const Vector4& weights1,
    uint16_t outJoints[4],
    Vector4& outWeights)
{
    std::array<JointInfluence, 8> influences{};
    int count = 0;

    for (int i = 0; i < 4; ++i)
    {
        const float w = getVec4Component(weights0, i);
        if (w > 0.0f)
            influences[count++] = { joints0[i], w };
    }

    for (int i = 0; i < 4; ++i)
    {
        const float w = getVec4Component(weights1, i);
        if (w > 0.0f)
            influences[count++] = { joints1[i], w };
    }

    std::sort(influences.begin(), influences.begin() + count,
        [](const JointInfluence& a, const JointInfluence& b)
        {
            return a.weight > b.weight;
        });

    for (int i = 0; i < 4; ++i)
        outJoints[i] = 0;
    outWeights = Vector4::Zero;

    const int keptCount = std::min(count, 4);
    float totalWeight = 0.0f;

    for (int i = 0; i < keptCount; ++i)
    {
        outJoints[i] = influences[i].joint;
        setVec4Component(outWeights, i, influences[i].weight);
        totalWeight += influences[i].weight;
    }

    if (totalWeight > 0.0f)
        outWeights /= totalWeight;
}

static std::string resolveNodeName(const tinygltf::Model& model, int nodeIdx)
{
    if (nodeIdx < 0 || nodeIdx >= (int)model.nodes.size()) return "";
    const std::string& n = model.nodes[nodeIdx].name;
    if (!n.empty()) return n;
    return "Node_" + std::to_string(nodeIdx);
}

static bool loadFloats(const tinygltf::Model& model, int accessorIdx, std::vector<float>& out)
{
    if (accessorIdx < 0 || accessorIdx >= (int)model.accessors.size()) return false;
    const auto& acc = model.accessors[accessorIdx];
    out.resize(acc.count);
    return loadAccessorData(reinterpret_cast<uint8_t*>(out.data()), sizeof(float), sizeof(float), (uint32_t)acc.count, model, accessorIdx);
}

static bool loadVec3(const tinygltf::Model& model, int accessorIdx, std::vector<Vector3>& out)
{
    if (accessorIdx < 0 || accessorIdx >= (int)model.accessors.size()) return false;
    const auto& acc = model.accessors[accessorIdx];
    out.resize(acc.count);
    return loadAccessorData(reinterpret_cast<uint8_t*>(out.data()), sizeof(Vector3), sizeof(Vector3), (uint32_t)acc.count, model, accessorIdx);
}

static bool loadQuat(const tinygltf::Model& model, int accessorIdx, std::vector<Quaternion>& out)
{
    if (accessorIdx < 0 || accessorIdx >= (int)model.accessors.size()) return false;
    const auto& acc = model.accessors[accessorIdx];
    out.resize(acc.count);
    return loadAccessorData(reinterpret_cast<uint8_t*>(out.data()), sizeof(Quaternion), sizeof(Quaternion), (uint32_t)acc.count, model, accessorIdx);
}

static bool loadMatrices(const tinygltf::Model& model, int accessorIdx, std::vector<Matrix>& out)
{
    if (accessorIdx < 0 || accessorIdx >= (int)model.accessors.size()) return false;
    const auto& acc = model.accessors[accessorIdx];
    out.resize(acc.count);
    return loadAccessorData(reinterpret_cast<uint8_t*>(out.data()), sizeof(Matrix), sizeof(Matrix), (uint32_t)acc.count, model, accessorIdx);
}

static bool loadJointIndices4(const tinygltf::Model& model, int accessorIdx,
    uint16_t* dst, uint32_t count, uint32_t dstStride)
{
    if (accessorIdx < 0 || accessorIdx >= (int)model.accessors.size()) return false;

    const tinygltf::Accessor& acc = model.accessors[accessorIdx];
    if (acc.type != TINYGLTF_TYPE_VEC4) return false;
    if (acc.bufferView < 0 || acc.bufferView >= (int)model.bufferViews.size()) return false;

    const tinygltf::BufferView& view = model.bufferViews[acc.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];

    const uint8_t* src = buffer.data.data() + view.byteOffset + acc.byteOffset;
    const size_t srcStride = acc.ByteStride(view);
    if (srcStride == 0) return false;

    for (uint32_t i = 0; i < count; ++i)
    {
        uint16_t* out = reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(dst) + i * dstStride);
        const uint8_t* in = src + i * srcStride;

        switch (acc.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        {
            const uint8_t* v = reinterpret_cast<const uint8_t*>(in);
            out[0] = v[0]; out[1] = v[1]; out[2] = v[2]; out[3] = v[3];
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            const uint16_t* v = reinterpret_cast<const uint16_t*>(in);
            out[0] = v[0]; out[1] = v[1]; out[2] = v[2]; out[3] = v[3];
            break;
        }
        default:
            return false;
        }
    }

    return true;
}

ImporterGltf::ImporterGltf(ImporterMesh* importerMesh,
    ImporterMaterial* importerMaterial,
    ImporterPrefab* importerPrefab,
    ImporterAnimation* importerAnimation,
    ImporterSkin* importerSkin,
    ImporterAnimationStateMachine* importerAnimationStateMachine)
    : m_importerMesh(importerMesh)
    , m_importerMaterial(importerMaterial)
    , m_importerPrefab(importerPrefab)
    , m_importerAnimation(importerAnimation)
    , m_importerSkin(importerSkin)
    , m_importerAnimationStateMachine(importerAnimationStateMachine)
{
}

AssetReference ImporterGltf::resolveOrGenerateReference(AssetType type, const uint8_t* data, size_t size)
{
    MD5Hash contentHash = INVALID_ASSET_ID;

    if (data && size > 0)
    {
        const std::vector<int8_t> hashInput(reinterpret_cast<const int8_t*>(data), reinterpret_cast<const int8_t*>(data) + size);
        contentHash = to_hex_string(computeMD5(hashInput));

        for (size_t i = 0; i < m_existingDeps.size(); ++i)
        {
            if (!m_existingDepsUsed[i]&& m_existingDeps[i].type == type && m_existingDeps[i].contentHash == contentHash)
            {
                m_existingDepsUsed[i] = true;
                DEBUG_LOG("[ImporterGltf] Reusing existing UID '%s' for unchanged sub-asset (type %u).", std::to_string(m_existingDeps[i].uid).c_str(), static_cast<unsigned>(type));
                return AssetReference(m_existingDeps[i].uid, contentHash, type);
            }
        }
    }

    return AssetReference(GenerateUID(), contentHash, type);
}


AssetReference ImporterGltf::resolveTexture(const tinygltf::Model& model, int texIndex) const
{
    if (texIndex < 0 || texIndex >= static_cast<int>(model.textures.size()))
    {
        return AssetReference{};
    }

    const tinygltf::Texture& tex = model.textures[texIndex];
    if (tex.source < 0 || tex.source >= static_cast<int>(model.images.size()))
    {
        return AssetReference{};
    }

    const tinygltf::Image& img = model.images[tex.source];
    if (img.uri.empty())
    {
        return AssetReference{};
    }

    const std::filesystem::path resolved = m_currentFilePath->parent_path() / img.uri;
    ModuleAssets* assets = app->getModuleAssets();

    // Re-use an already-imported texture when possible.
    const UID existingUID = assets->findUID(resolved);

    if (isValidUID(existingUID))
    {
        std::filesystem::path metaPath = resolved;
        Metadata::getMetadataPath(metaPath);
        Metadata meta;
        if (assets->loadMetaFile(metaPath, meta))
        {
            return AssetReference(meta.uid, meta.contentHash, meta.type);
        }

        AssetReference ref(existingUID);
        assets->importAsset(resolved, ref);
        return ref;
    }

    AssetReference ref;
    assets->importAsset(resolved, ref);
    return ref;
}


bool ImporterGltf::canImport(const std::filesystem::path& path) const
{
    return path.extension().string() == GLTF_EXTENSION;
}

Asset* ImporterGltf::createAssetInstance(AssetReference& ref) const
{
    return new PrefabAsset(ref);
}

bool ImporterGltf::loadExternal(const std::filesystem::path& path, tinygltf::Model& out)
{
    tinygltf::TinyGLTF ctx;
    std::string err, warn;
    if (!ctx.LoadASCIIFromFile(&out, &err, &warn, path.string().c_str()))
    {
        DEBUG_ERROR("[ImporterGltf] Failed to load '%s': %s", path.string().c_str(), err.c_str());
        return false;
    }

    DEBUG_LOG("IMPORTED");
    m_currentFilePath = &path;
    return true;
}


void ImporterGltf::importTyped(const tinygltf::Model& model, PrefabAsset* dst)
{
    ModuleAssets* assets = app->getModuleAssets();

    m_existingDeps.clear();
    m_existingDepsUsed.clear();

    {
        std::filesystem::path metaPath = *m_currentFilePath;
        Metadata::getMetadataPath(metaPath);
        Metadata existingMeta;
        if (assets->loadMetaFile(metaPath, existingMeta))
        {
            m_existingDeps = existingMeta.m_dependencies;
            m_existingDepsUsed.assign(m_existingDeps.size(), false);
        }
    }


    std::vector<AssetReference> materialRefs(model.materials.size());
    for (int i = 0; i < static_cast<int>(model.materials.size()); ++i)
    {
        AssetReference tempRef;
        MaterialAsset matAsset(tempRef);
        loadMaterial(model, model.materials[i], &matAsset);

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerMaterial->save(&matAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        AssetReference matRef = resolveOrGenerateReference(AssetType::MATERIAL, rawBuf, static_cast<size_t>(size));
        Metadata meta; meta.uid = matRef.m_uid; meta.type = matRef.m_type; meta.contentHash = matRef.m_libId;
        assets->registerSubAsset(meta, dst->m_reference.m_uid, rawBuf, static_cast<size_t>(size));
        materialRefs[i] = matRef;
    }


    std::vector<AssetReference> meshRefs(model.meshes.size());
    for (int i = 0; i < static_cast<int>(model.meshes.size()); ++i)
    {
        AssetReference tempRef;
        MeshAsset meshAsset(tempRef);
        for (const tinygltf::Primitive& prim : model.meshes[i].primitives)
        {
            const AssetReference matRef = (prim.material >= 0 && prim.material < static_cast<int>(materialRefs.size())) ? materialRefs[prim.material] : AssetReference{};
            loadMesh(model, prim, &meshAsset, matRef);
        }

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerMesh->save(&meshAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        AssetReference meshRef = resolveOrGenerateReference(AssetType::MESH, rawBuf, static_cast<size_t>(size));
        Metadata meta; meta.uid = meshRef.m_uid; meta.type = meshRef.m_type; meta.contentHash = meshRef.m_libId;
        assets->registerSubAsset(meta, dst->m_reference.m_uid, rawBuf, static_cast<size_t>(size));
        meshRefs[i] = meshRef;
    }


    std::vector<AssetReference> animationRefs(model.animations.size());
    for (int i = 0; i < static_cast<int>(model.animations.size()); ++i)
    {
        AssetReference tempRef;
        AnimationAsset animAsset(tempRef);
        loadAnimation(model, model.animations[i], &animAsset);

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerAnimation->save(&animAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        AssetReference animRef = resolveOrGenerateReference(AssetType::ANIMATION, rawBuf, static_cast<size_t>(size));
        Metadata meta; meta.uid = animRef.m_uid; meta.type = animRef.m_type; meta.contentHash = animRef.m_libId;
        assets->registerSubAsset(meta, dst->m_reference.m_uid, rawBuf, static_cast<size_t>(size));
        animationRefs[i] = animRef;
    }

    if (!animationRefs.empty())
    {
        buildDefaultStateMachine(model, animationRefs, dst);
    }


    std::vector<AssetReference> skinRefs(model.skins.size());
    for (int i = 0; i < static_cast<int>(model.skins.size()); ++i)
    {
        AssetReference tempRef;
        SkinAsset skinAsset(tempRef);
        loadSkin(model, model.skins[i], &skinAsset);

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerSkin->save(&skinAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        AssetReference skinRef = resolveOrGenerateReference(AssetType::SKIN, rawBuf, static_cast<size_t>(size));
        Metadata meta; meta.uid = skinRef.m_uid; meta.type = skinRef.m_type; meta.contentHash = skinRef.m_libId;
        assets->registerSubAsset(meta, dst->m_reference.m_uid, rawBuf, static_cast<size_t>(size));
        skinRefs[i] = skinRef;
    }

    std::vector<std::unique_ptr<GameObject>> tempObjects;

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
    {
        root = buildNode(rootNodes[0], nullptr, model, meshRefs, materialRefs, skinRefs, tempObjects);
    }
    else
    {
        root = makeNode(prefabName, tempObjects);
        for (int idx : rootNodes)
        {
            buildNode(idx, root, model, meshRefs, materialRefs, skinRefs, tempObjects);
        }
    }

    PrefabData& data = dst->getData();
    data.m_json = PrefabSerializer::buildPrefabJSON(root, *m_currentFilePath);
    data.m_sourcePath = *m_currentFilePath;
    data.m_name = prefabName;
    data.m_assetUID = dst->m_reference.m_uid;

    m_currentFilePath = nullptr;
    m_existingDeps.clear();
    m_existingDepsUsed.clear();
}


uint64_t ImporterGltf::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    return m_importerPrefab->save(src, outBuffer);
}

void ImporterGltf::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    m_importerPrefab->load(buffer, dst);
}

void ImporterGltf::loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
    MeshAsset* mesh, const AssetReference& materialRef)
{
    const uint32_t baseVertex = static_cast<uint32_t>(mesh->vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(mesh->indices.size());

    auto itPos = primitive.attributes.find("POSITION");
    if (itPos == primitive.attributes.end()) return;

    const tinygltf::Accessor& posAcc = model.accessors[itPos->second];
    const uint32_t            vertexCount = static_cast<uint32_t>(posAcc.count);

    mesh->vertices.resize(baseVertex + vertexCount);
    uint8_t* vBase = reinterpret_cast<uint8_t*>(mesh->vertices.data() + baseVertex);

    loadAccessorData(vBase + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), vertexCount, model, itPos->second);
    loadAccessorData(vBase + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), vertexCount, model, primitive.attributes, "NORMAL");
    loadAccessorData(vBase + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), vertexCount, model, primitive.attributes, "TEXCOORD_0");

    auto itJoints = primitive.attributes.find("JOINTS_0");
    if (itJoints != primitive.attributes.end())
    {
        loadJointIndices4(model, itJoints->second,
            reinterpret_cast<uint16_t*>(vBase + offsetof(Vertex, joints)),
            vertexCount, sizeof(Vertex));
    }

    auto itWeights = primitive.attributes.find("WEIGHTS_0");
    if (itWeights != primitive.attributes.end())
    {
        loadAccessorData(vBase + offsetof(Vertex, weights), sizeof(Vector4), sizeof(Vertex),
            vertexCount, model, itWeights->second);
    }

    auto itJoints1 = primitive.attributes.find("JOINTS_1");
    auto itWeights1 = primitive.attributes.find("WEIGHTS_1");

    const bool hasJoints1 = (itJoints1 != primitive.attributes.end());
    const bool hasWeights1 = (itWeights1 != primitive.attributes.end());

    if (hasJoints1 != hasWeights1)
    {
        DEBUG_WARN("[ImporterGltf] Primitive has only one of JOINTS_1 / WEIGHTS_1. Extra influences will be ignored.");
    }
    else if (hasJoints1 && hasWeights1)
    {
        std::vector<std::array<uint16_t, 4>> extraJoints(vertexCount);
        std::vector<Vector4> extraWeights(vertexCount, Vector4::Zero);

        const bool jointsLoaded = loadJointIndices4(model, itJoints1->second,
            reinterpret_cast<uint16_t*>(extraJoints.data()),
            vertexCount, sizeof(std::array<uint16_t, 4>));

        const bool weightsLoaded = loadAccessorData(
            reinterpret_cast<uint8_t*>(extraWeights.data()),
            sizeof(Vector4), sizeof(Vector4), vertexCount, model, itWeights1->second);

        if (!jointsLoaded || !weightsLoaded)
        {
            DEBUG_WARN("[ImporterGltf] Failed to load JOINTS_1 / WEIGHTS_1. Extra influences will be ignored.");
        }
        else
        {
            Vertex* vertices = mesh->vertices.data() + baseVertex;
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                uint16_t collapsedJoints[4] = {};
                Vector4  collapsedWeights = Vector4::Zero;

                collapseToTop4Influences(
                    vertices[i].joints, vertices[i].weights,
                    extraJoints[i].data(), extraWeights[i],
                    collapsedJoints, collapsedWeights);

                for (int j = 0; j < 4; ++j)
                    vertices[i].joints[j] = collapsedJoints[j];
                vertices[i].weights = collapsedWeights;
            }
        }
    }

    uint32_t indexCount = 0, componentSize = 0;
    if (primitive.indices >= 0)
    {
        const tinygltf::Accessor& idxAcc = model.accessors[primitive.indices];
        const int ct = idxAcc.componentType;
        if (ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE
            || ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT
            || ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
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
    mesh->submeshes.push_back(submesh);

    if (posAcc.minValues.size() == 3 && posAcc.maxValues.size() == 3)
    {
        const Vector3 bMin((float)posAcc.minValues[0], (float)posAcc.minValues[1], (float)posAcc.minValues[2]);
        const Vector3 bMax((float)posAcc.maxValues[0], (float)posAcc.maxValues[1], (float)posAcc.maxValues[2]);
        mesh->boundsCenter = (bMin + bMax) * 0.5f;
        mesh->boundsExtents = (bMax - bMin) * 0.5f;
    }
}

void ImporterGltf::loadAnimation(const tinygltf::Model& model,
    const tinygltf::Animation& anim, AnimationAsset* outAnim)
{
    outAnim->m_name = anim.name.empty() ? "Anim" : anim.name;
    outAnim->m_durationSeconds = 0.0f;
    outAnim->m_channels.clear();

    for (const auto& ch : anim.channels)
    {
        if (ch.target_node < 0) continue;
        if (ch.sampler < 0 || ch.sampler >= (int)anim.samplers.size()) continue;

        const std::string nodeName = resolveNodeName(model, ch.target_node);
        if (nodeName.empty()) continue;

        const auto& sampler = anim.samplers[ch.sampler];

        std::vector<float> times;
        if (!loadFloats(model, sampler.input, times) || times.empty()) continue;

        outAnim->m_durationSeconds = std::max(outAnim->m_durationSeconds, times.back());

        AnimChannel& dst = outAnim->m_channels[nodeName];

        if (ch.target_path == "translation")
        {
            std::vector<Vector3> values;
            if (!loadVec3(model, sampler.output, values)) continue;
            const size_t n = std::min(times.size(), values.size());
            dst.posKeys.reserve(dst.posKeys.size() + n);
            for (size_t i = 0; i < n; ++i)
            {
                dst.posKeys.push_back({ times[i], values[i] });
            }
        }
        else if (ch.target_path == "rotation")
        {
            std::vector<Quaternion> values;
            if (!loadQuat(model, sampler.output, values)) continue;
            const size_t n = std::min(times.size(), values.size());
            dst.rotKeys.reserve(dst.rotKeys.size() + n);
            for (size_t i = 0; i < n; ++i)
            {
                dst.rotKeys.push_back({ times[i], values[i] });
            }
        }
        else if (ch.target_path == "scale")
        {
            std::vector<Vector3> values;
            if (!loadVec3(model, sampler.output, values)) continue;
            const size_t n = std::min(times.size(), values.size());
            dst.scaleKeys.reserve(dst.scaleKeys.size() + n);
            for (size_t i = 0; i < n; ++i)
            {
                dst.scaleKeys.push_back({ times[i], values[i] });
            }
        }
    }
}


void ImporterGltf::buildDefaultStateMachine(const tinygltf::Model& model, const std::vector<AssetReference>& animationRefs, PrefabAsset* dst)
{
    if (!m_currentFilePath || !dst || animationRefs.empty())
        return;

    ModuleAssets* assets = app->getModuleAssets();
    if (!assets)
        return;

    AssetReference tempRef;
    AnimationStateMachineAsset stateMachineAsset(tempRef);
    stateMachineAsset.m_name = m_currentFilePath->stem().string() + "_StateMachine";
    stateMachineAsset.m_defaultStateName.clear();
    stateMachineAsset.m_clips.clear();
    stateMachineAsset.m_states.clear();
    stateMachineAsset.m_transitions.clear();

    for (size_t i = 0; i < model.animations.size() && i < animationRefs.size(); ++i)
    {
        const tinygltf::Animation& anim = model.animations[i];
        const AssetReference& animRef = animationRefs[i];

        if (!isValidUID(animRef.m_uid))
            continue;

        const std::string baseName = anim.name.empty() ? ("Anim_" + std::to_string(i)) : anim.name;

        AnimationStateMachineClip clip;
        clip.name = baseName;
        clip.animationUID = animRef;
        clip.loop = true;

        AnimationStateMachineState state;
        state.name = baseName;
        state.clipName = clip.name;
        state.speed = 1.0f;

        stateMachineAsset.m_clips.push_back(std::move(clip));
        stateMachineAsset.m_states.push_back(std::move(state));
    }

    if (stateMachineAsset.m_states.empty())
        return;

    // Pick the default state: prefer one whose name contains "idle".
    for (const AnimationStateMachineState& state : stateMachineAsset.m_states)
    {
        std::string lower = state.name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (lower.find("idle") != std::string::npos)
        {
            stateMachineAsset.m_defaultStateName = state.name;
            break;
        }
    }

    if (stateMachineAsset.m_defaultStateName.empty())
    {
        stateMachineAsset.m_defaultStateName = stateMachineAsset.m_states.front().name;
    }

    uint8_t* rawBuf = nullptr;
    const uint64_t size = m_importerAnimationStateMachine->save(&stateMachineAsset, &rawBuf);
    std::unique_ptr<uint8_t[]> guard(rawBuf);

    AssetReference smRef = resolveOrGenerateReference(AssetType::ANIMATION_STATE_MACHINE, rawBuf, static_cast<size_t>(size));
    Metadata meta;
    meta.uid = smRef.m_uid;
    meta.type = smRef.m_type;
    meta.contentHash = smRef.m_libId;
    assets->registerSubAsset(meta, dst->m_reference.m_uid, rawBuf, static_cast<size_t>(size));
}


void ImporterGltf::loadSkin(const tinygltf::Model& model,
    const tinygltf::Skin& skin, SkinAsset* outSkin)
{
    outSkin->m_name = skin.name.empty() ? "Skin" : skin.name;
    outSkin->m_joints.clear();

    std::vector<Matrix> inverseBindMatrices;
    if (skin.inverseBindMatrices >= 0)
        loadMatrices(model, skin.inverseBindMatrices, inverseBindMatrices);

    outSkin->m_joints.reserve(skin.joints.size());

    for (size_t i = 0; i < skin.joints.size(); ++i)
    {
        const int nodeIdx = skin.joints[i];

        SkinJoint joint;
        joint.nodeName = resolveNodeName(model, nodeIdx);
        joint.inverseBindMatrix = (i < inverseBindMatrices.size())
            ? inverseBindMatrices[i]
            : Matrix::Identity;

        outSkin->m_joints.push_back(std::move(joint));
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

    mat->metallicFactor = static_cast<uint32_t>(pbr.metallicFactor);
    mat->roughnessFactor = static_cast<uint32_t>(pbr.roughnessFactor);

    // All texture slots are now AssetReference; resolveTexture returns one directly.
    mat->baseMap = resolveTexture(model, pbr.baseColorTexture.index);
    mat->metallicRoughnessMap = resolveTexture(model, pbr.metallicRoughnessTexture.index);
    mat->normalMap = resolveTexture(model, material.normalTexture.index);
    mat->occlusionMap = resolveTexture(model, material.occlusionTexture.index);
    mat->emissiveMap = resolveTexture(model, material.emissiveTexture.index);
    mat->isEmissive = mat->emissiveMap.isValid();

}


GameObject* ImporterGltf::makeNode(const std::string& name, std::vector<std::unique_ptr<GameObject>>& tempObjects) const
{
    auto go = std::make_unique<GameObject>(GenerateUID(), GenerateUID());
    go->SetName(name);
    GameObject* raw = go.get();
    tempObjects.push_back(std::move(go));
    return raw;
}

GameObject* ImporterGltf::buildNode(int nodeIdx, GameObject* parent,
    const tinygltf::Model& model,
    const std::vector<AssetReference>& meshRefs,
    const std::vector<AssetReference>& materialRefs,
    const std::vector<AssetReference>& skinRefs,
    std::vector<std::unique_ptr<GameObject>>& tempObjects) const
{
    const tinygltf::Node& gNode = model.nodes[nodeIdx];
    const std::string name = gNode.name.empty() ? ("Node_" + std::to_string(nodeIdx)) : gNode.name;

    GameObject* go = makeNode(name, tempObjects);
    Transform* tf = go->GetTransform();

    if (gNode.translation.size() == 3)
    {
        tf->setPosition(Vector3((float)gNode.translation[0], (float)gNode.translation[1], (float)gNode.translation[2]));
    }

    if (gNode.rotation.size() == 4)
    {
        tf->setRotation(Quaternion((float)gNode.rotation[0], (float)gNode.rotation[1], (float)gNode.rotation[2], (float)gNode.rotation[3]));
    }

    if (gNode.scale.size() == 3)
    {
        tf->setScale(Vector3((float)gNode.scale[0], (float)gNode.scale[1], (float)gNode.scale[2]));
    }

    if (parent)
    {
        Transform* parentTf = parent->GetTransform();
        tf->setRoot(parentTf);
        parentTf->addChild(go);
    }

    if (gNode.mesh >= 0 && gNode.mesh < static_cast<int>(meshRefs.size()))
    {
        auto* mr = static_cast<MeshRenderer*>(go->AddComponentWithUID(ComponentType::MODEL, GenerateUID()));
        if (mr)
        {
            // Assign full AssetReferences — component slots now hold AssetReference.
            AssetReference meshRef = meshRefs[gNode.mesh];
            mr->setMeshReference(meshRef);

            if (gNode.skin >= 0 && gNode.skin < static_cast<int>(skinRefs.size()))
            {
                AssetReference skinRef = skinRefs[gNode.skin];
                mr->setSkinReference(skinRef);
            }

            const auto& prims = model.meshes[gNode.mesh].primitives;

            for (const tinygltf::Primitive& prim : prims)
            {
                AssetReference matRef =
                    (prim.material >= 0 && prim.material < static_cast<int>(materialRefs.size()))
                    ? materialRefs[prim.material]
                    : AssetReference{};

                mr->addMaterialReference(matRef);
            }
        }
    }

    for (int childIdx : gNode.children)
    {
        buildNode(childIdx, go, model, meshRefs, materialRefs, skinRefs, tempObjects);
    }

    return go;
}