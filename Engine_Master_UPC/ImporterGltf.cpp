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
#include "MD5.h"

#include "ImporterMesh.h"
#include "ImporterMaterial.h"
#include "ImporterPrefab.h"
#include "ImporterAnimation.h"

#include <functional>
#include "AnimationAsset.h"
#include <unordered_map>
#include <assert.h>

static const DXGI_FORMAT INDEX_FORMATS[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };

static MD5Hash resolveTexture(const tinygltf::Model& model, int texIndex, const std::filesystem::path* modelPath)
{
    if (texIndex < 0 || texIndex >= static_cast<int>(model.textures.size()))
    {
        return INVALID_ASSET_ID;
    }

    const tinygltf::Texture& tex = model.textures[texIndex];
    if (tex.source < 0 || tex.source >= static_cast<int>(model.images.size()))
    {
        return INVALID_ASSET_ID;
    }

    const tinygltf::Image& img = model.images[tex.source];
    if (img.uri.empty()) 
    {
        return INVALID_ASSET_ID;
    }
    std::filesystem::path resolved = modelPath->parent_path() / img.uri;
    MD5Hash uid = app->getModuleAssets()->findUID(resolved);
    if (!isValidAsset(uid)) 
    {
        app->getModuleAssets()->importAsset(resolved, uid);
    }
    return uid;
}

ImporterGltf::ImporterGltf(ImporterMesh& importerMesh, ImporterMaterial& importerMaterial, ImporterPrefab& importerPrefab, ImporterAnimation& importerAnimation) : m_importerMesh(importerMesh), m_importerMaterial(importerMaterial), m_importerPrefab(importerPrefab), m_importerAnimation(importerAnimation)
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

void ImporterGltf::importTyped(const tinygltf::Model& model, PrefabAsset* dst)
{
    ModuleAssets* assets = app->getModuleAssets();

    //Materials
    std::vector<MD5Hash> materialUIDs(model.materials.size(), INVALID_ASSET_ID);
    for (int i = 0; i < static_cast<int>(model.materials.size()); ++i)
    {
        const MD5Hash matUID = computeMD5(m_currentFilePath->string() + "?mat=" + std::to_string(i));
        MaterialAsset matAsset(matUID);
        loadMaterial(model, model.materials[i], &matAsset);

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerMaterial.save(&matAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        Metadata meta; meta.uid = matUID; meta.type = AssetType::MATERIAL;
        assets->registerSubAsset(meta, dst->m_uid, rawBuf, static_cast<size_t>(size));
        materialUIDs[i] = matUID;
    }

    //Meshes
    std::vector<MD5Hash> meshUIDs(model.meshes.size(), INVALID_ASSET_ID);
    for (int i = 0; i < static_cast<int>(model.meshes.size()); ++i)
    {
        const MD5Hash meshUID = computeMD5(m_currentFilePath->string() + "?mesh=" + std::to_string(i));
        MeshAsset meshAsset(meshUID);
        for (const tinygltf::Primitive& prim : model.meshes[i].primitives)
        {
            const MD5Hash matUID = (prim.material >= 0 && prim.material < static_cast<int>(materialUIDs.size())) ? materialUIDs[prim.material] : INVALID_ASSET_ID;
            loadMesh(model, prim, &meshAsset, matUID);
        }

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerMesh.save(&meshAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        Metadata meta; meta.uid = meshUID; meta.type = AssetType::MESH;
        assets->registerSubAsset(meta, dst->m_uid, rawBuf, static_cast<size_t>(size));
        meshUIDs[i] = meshUID;
    }

    //Animations
	std::vector<MD5Hash> animationUIDs(model.animations.size(), INVALID_ASSET_ID);
    for (int i = 0; i < static_cast<int>(model.animations.size()); ++i)
    {
		const MD5Hash animUID = computeMD5(m_currentFilePath->string() + "?anim=" + std::to_string(i));
		AnimationAsset animAsset(animUID);
		loadAnimation(model, model.animations[i], &animAsset);

        uint8_t* rawBuf = nullptr;
        const uint64_t size = m_importerAnimation.save(&animAsset, &rawBuf);
        std::unique_ptr<uint8_t[]> guard(rawBuf);

        Metadata meta; meta.uid = animUID; meta.type = AssetType::ANIMATION;
		assets->registerSubAsset(meta, dst->m_uid, rawBuf, static_cast<size_t>(size));
		animationUIDs[i] = animUID;
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
        root = buildNode(rootNodes[0], nullptr, model, meshUIDs, materialUIDs, tempObjects);
    }
    else
    {
        root = makeNode(prefabName, tempObjects);
        for (int idx : rootNodes)
        {
            buildNode(idx, root, model, meshUIDs, materialUIDs, tempObjects);
        }
    }

    PrefabData& data = dst->getData();
    data.m_json = PrefabManager::buildPrefabJSON(root, *m_currentFilePath);
    data.m_sourcePath = *m_currentFilePath;
    data.m_name = prefabName;
    data.m_assetUID = dst->m_uid;
    data.m_prefabUID = root->GetID();

    m_currentFilePath = nullptr;
    // tempObjects destroyed here — all temporary GameObjects are freed.
}


uint64_t ImporterGltf::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    return m_importerPrefab.save(src, outBuffer);
}

void ImporterGltf::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    m_importerPrefab.load(buffer, dst);
}

void ImporterGltf::loadMaterial(const tinygltf::Model& model,
    const tinygltf::Material& material,
    MaterialAsset* mat)
{
    const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;
    mat->baseColour = Color( float(pbr.baseColorFactor[0]), float(pbr.baseColorFactor[1]), float(pbr.baseColorFactor[2]), float(pbr.baseColorFactor[3]));
    mat->metallicFactor = static_cast<uint32_t>(pbr.metallicFactor * 255.0f);
    mat->baseMap = resolveTexture(model, pbr.baseColorTexture.index, m_currentFilePath);
    mat->metallicRoughnessMap = resolveTexture(model, pbr.metallicRoughnessTexture.index, m_currentFilePath);
    mat->normalMap = resolveTexture(model, material.normalTexture.index, m_currentFilePath);
    mat->occlusionMap = resolveTexture(model, material.occlusionTexture.index, m_currentFilePath);
    mat->emissiveMap = resolveTexture(model, material.emissiveTexture.index, m_currentFilePath);
    mat->isEmissive = isValidAsset(mat->emissiveMap);
}

void ImporterGltf::loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshAsset* mesh, const MD5Hash& materialUID)
{
    const uint32_t baseVertex = static_cast<uint32_t>(mesh->vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(mesh->indices.size());

    auto itPos = primitive.attributes.find("POSITION");
    if (itPos == primitive.attributes.end()) return;

    const tinygltf::Accessor& posAcc = model.accessors[itPos->second];
    const uint32_t vertexCount = static_cast<uint32_t>(posAcc.count);

    mesh->vertices.resize(baseVertex + vertexCount);
    uint8_t* vBase = reinterpret_cast<uint8_t*>(mesh->vertices.data() + baseVertex);

    loadAccessorData(vBase + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), vertexCount, model, itPos->second);
    loadAccessorData(vBase + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), vertexCount, model, primitive.attributes, "NORMAL");
    loadAccessorData(vBase + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), vertexCount, model, primitive.attributes, "TEXCOORD_0");

    uint32_t indexCount = 0, componentSize = 0;
    if (primitive.indices >= 0)
    {
        const tinygltf::Accessor& idxAcc = model.accessors[primitive.indices];
        const int ct = idxAcc.componentType;
        if (ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE || ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT || ct == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
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

void ImporterGltf::loadAnimation(const tinygltf::Model& model, const tinygltf::Animation& animation, AnimationAsset* anim)
{
	const auto& channels = animation.channels;
    for (int i = 0; i < static_cast<int>(channels.size()); ++i)
    {
        const tinygltf::AnimationChannel& channel = channels[i];
        const tinygltf::Node& targetNode = model.nodes[channel.target_node];

        const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];

        const tinygltf::Accessor& inputAcc = model.accessors[sampler.input];
        const tinygltf::BufferView& inputView = model.bufferViews[inputAcc.bufferView];
        const tinygltf::Buffer& inputBuffer = model.buffers[inputView.buffer];
        const unsigned char* inputPtr = inputBuffer.data.data() +
            inputView.byteOffset +
            inputAcc.byteOffset;
        const float* timestamps = reinterpret_cast<const float*>(inputPtr);

        const tinygltf::Accessor& outputAcc = model.accessors[sampler.output];
        const tinygltf::BufferView& outputView = model.bufferViews[outputAcc.bufferView];
        const tinygltf::Buffer& outputBuffer = model.buffers[outputView.buffer];
        const unsigned char* outputPtr = outputBuffer.data.data() +
            outputView.byteOffset +
            outputAcc.byteOffset;
        const float* properties = reinterpret_cast<const float*>(outputPtr);

        if (channel.target_path == "translation")
        {
            if (outputAcc.type != TINYGLTF_TYPE_VEC3)
            {
                DEBUG_WARN("Output accessor type does not match with target property type.");
                return;
            }
            std::vector<float> channelPosTimeStamps;
            for (int j = 0; j < inputAcc.count; ++j)
            {
                channelPosTimeStamps.push_back(timestamps[j]);
                if(anim->duration < timestamps[j])
                {
                    anim->duration = timestamps[j];
				}
            }

            auto& channelData = anim->channels[targetNode.name];
            if(!channelData.posTimeStamps)
            {
                channelData.posTimeStamps = std::make_unique<float[]>(inputAcc.count);
            }
            std::copy(channelPosTimeStamps.begin(),
                channelPosTimeStamps.end(),
                channelData.posTimeStamps.get());

            std::vector<Vector3> channelPositions;
            for (int j = 0; j < outputAcc.count; ++j)
            {
                channelPositions.emplace_back(
                    properties[j * 3 + 0],
                    properties[j * 3 + 1],
                    properties[j * 3 + 2]);
            }

            if(!channelData.positions)
            {
                channelData.positions = std::make_unique<Vector3[]>(outputAcc.count);
			}
            std::copy(reinterpret_cast<const Vector3*>(properties),
                reinterpret_cast<const Vector3*>(properties) + outputAcc.count,
                channelData.positions.get());
		    channelData.numPositions = outputAcc.count;
        }
        if (channel.target_path == "rotation")
        {
            if (outputAcc.type != TINYGLTF_TYPE_VEC4)
            {
                DEBUG_WARN("Output accessor type does not match with target property type.");
                return;
            }
            std::vector<float> channelRotTimeStamps;
            for (int j = 0; j < inputAcc.count; ++j)
            {
				channelRotTimeStamps.push_back(timestamps[j]);
            }

            auto& channelData = anim->channels[targetNode.name];
            if(!channelData.rotTimeStamps)
            {
                channelData.rotTimeStamps = std::make_unique<float[]>(inputAcc.count);
			}
            std::copy(timestamps, 
                timestamps + inputAcc.count, 
                channelData.rotTimeStamps.get());

            std::vector<Quaternion> channelRotations;
            for (int j = 0; j < outputAcc.count; ++j)
            {
                channelRotations.emplace_back(
                    properties[j * 4 + 0],
                    properties[j * 4 + 1],
                    properties[j * 4 + 2],
                    properties[j * 4 + 3]);
            }

            if (!channelData.rotations)
            {
                channelData.rotations = std::make_unique<Quaternion[]>(outputAcc.count);
            }
            std::copy(reinterpret_cast<const Quaternion*>(properties),
                reinterpret_cast<const Quaternion*>(properties) + outputAcc.count,
                channelData.rotations.get());
            channelData.numRotations = outputAcc.count;
        }
        /* Scales optional
        if (channel.target_path == "scale")
        {
            if (outputAcc.type != TINYGLTF_TYPE_VEC3)
            {
                DEBUG_WARN("Output accessor type does not match with target property type.");
                return;
            }
            std::vector<float> channelScaleTimeStamps;
            for (int j = 0; j < inputAcc.count; ++j)
            {
                //anim->channels[targetNode.name].scaleTimeStamps.push_back(timestamps[j]);
            }
            auto& channelData = anim->channels[targetNode.name];
            std::copy(timestamps, timestamps + inputAcc.count, channelData.scaleTimeStamps.get());
            std::vector<Vector3> channelScales;
            for (int j = 0; j < outputAcc.count; ++j)
            {
                //anim->channels[targetNode.name].scales.push_back(Vector3(
                //    properties[j * 3 + 0],
                //    properties[j * 3 + 1],
                //    properties[j * 3 + 2]));
                channelScales.emplace_back(
                    properties[j * 3 + 0],
                    properties[j * 3 + 1],
                    properties[j * 3 + 2]);
            }
            std::copy(reinterpret_cast<const Vector3*>(properties),
                reinterpret_cast<const Vector3*>(properties) + outputAcc.count,
                channelData.scales.get());
        }*/
    }
}

GameObject* ImporterGltf::makeNode(const std::string& name, std::vector<std::unique_ptr<GameObject>>& tempObjects) const
{
    auto go = std::make_unique<GameObject>(GenerateUID(), GenerateUID());
    go->SetName(name);
    GameObject* raw = go.get();
    tempObjects.push_back(std::move(go));
    return raw;
}

GameObject* ImporterGltf::buildNode(int nodeIdx, GameObject* parent, const tinygltf::Model& model, const std::vector<MD5Hash>& meshUIDs, const std::vector<MD5Hash>& materialUIDs, std::vector<std::unique_ptr<GameObject>>& tempObjects) const
{
    const tinygltf::Node& gNode = model.nodes[nodeIdx];
    const std::string name = gNode.name.empty()
        ? ("Node_" + std::to_string(nodeIdx)) : gNode.name;

    GameObject* go = makeNode(name, tempObjects);
    Transform* tf = go->GetTransform();

    if (gNode.translation.size() == 3)
    {
        tf->setPosition(Vector3( (float)gNode.translation[0], (float)gNode.translation[1], (float)gNode.translation[2]));
    }

    if (gNode.rotation.size() == 4)
    {
        tf->setRotation(Quaternion( (float)gNode.rotation[0], (float)gNode.rotation[1], (float)gNode.rotation[2], (float)gNode.rotation[3]));
    }

    if (gNode.scale.size() == 3)
    {
        tf->setScale(Vector3( (float)gNode.scale[0], (float)gNode.scale[1], (float)gNode.scale[2]));
    }

    if (parent)
    {
        Transform* parentTf = parent->GetTransform();
        tf->setRoot(parentTf);
        parentTf->addChild(go);
    }

    if (gNode.mesh >= 0 && gNode.mesh < static_cast<int>(meshUIDs.size()))
    {
        auto* mr = static_cast<MeshRenderer*>(go->AddComponentWithUID(ComponentType::MODEL, GenerateUID()));
        if (mr)
        {
            mr->getMeshReference() = meshUIDs[gNode.mesh];
            const auto& prims = model.meshes[gNode.mesh].primitives;
            if (!prims.empty() && prims[0].material >= 0 && prims[0].material < static_cast<int>(materialUIDs.size()))
            {
                for (const tinygltf::Primitive& prim : prims)
                {
                    mr->getMaterialsReference().push_back(materialUIDs[prim.material]);
                }
            }
        }
    }

    for (int childIdx : gNode.children)
    {
        buildNode(childIdx, go, model, meshUIDs, materialUIDs, tempObjects);
    }

    return go;
}