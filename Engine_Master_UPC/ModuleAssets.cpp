#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "Importer.h"
#include "ImporterNative.h"
#include "ImporterMesh.h"
#include "ImporterMaterial.h"
#include "ImporterTexture.h"
#include "ImporterPrefab.h"
#include "ImporterAnimation.h"
#include "ImporterSkin.h"
#include "ImporterAnimationStateMachine.h"
#include "ImporterGltf.h"
#include "ImporterFont.h"
#include "MD5.h"

#include "PrefabSerializer.h"
#include "PrefabAsset.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "ModuleScene.h"

#include "Asset.h"
#include "AnimationStateMachineAsset.h"
#include "Metadata.h"
#include "UID.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"
#include <fstream>

#include <filesystem>
#include <FileIO.h>
#include <AssetDialogFilter.h>
#include <FileDialog.h>

using namespace rapidjson;
namespace fs = std::filesystem;

bool ModuleAssets::init()
{
    m_registry = std::make_unique<AssetRegistry>();

    m_importers.push_back(m_importerTexture               = new ImporterTexture());
    m_importers.push_back(m_importerMesh                  = new ImporterMesh());
    m_importers.push_back(m_importerMaterial              = new ImporterMaterial());
    m_importers.push_back(m_importerPrefab                = new ImporterPrefab());
    m_importers.push_back(m_importerAnimation             = new ImporterAnimation());
    m_importers.push_back(m_importerSkin                  = new ImporterSkin());
    m_importers.push_back(m_importerAnimationStateMachine = new ImporterAnimationStateMachine());
    m_importers.push_back(m_importerGltf                  = new ImporterGltf(m_importerMesh, m_importerMaterial, m_importerPrefab, m_importerAnimation, m_importerSkin, m_importerAnimationStateMachine));
    m_importers.push_back(m_importerFont = new ImporterFont());

    m_scanner         = std::make_unique<AssetScanner>(m_registry.get());
    m_contentRegistry = std::make_unique<ContentRegistry>(m_registry.get());

    refresh();
    return true;
}

void ModuleAssets::postRender()
{
    flushDialogRequests();
}

bool ModuleAssets::cleanUp()
{
    m_assets.clear();
    for (auto it = m_importers.rbegin(); it != m_importers.rend(); ++it)
        delete *it;
    return true;
}

Importer* ModuleAssets::findImporter(const std::filesystem::path& filePath) const
{
    for (auto& importer : m_importers)
    {
        if (importer->canImport(filePath)) return importer;
    }
    return nullptr;
}

Importer* ModuleAssets::findImporter(AssetType type) const
{
    for (auto& importer : m_importers)
    {
        if (importer->getAssetType() == type) return importer;
    }
    return nullptr;
}

bool ModuleAssets::canImport(const std::filesystem::path& sourcePath) const
{
    return findImporter(sourcePath) != nullptr;
}

void ModuleAssets::importAsset(const std::filesystem::path& sourcePath, UID& uid)
{
    Importer* importer = findImporter(sourcePath);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] No importer found for '%s'.", sourcePath.string().c_str());
        uid = INVALID_UID;
        return;
    }

    const bool isReimport = isValidUID(uid);
    if (!isReimport)
    {
        uid = GenerateUID();
    }

    std::unique_ptr<Asset> asset(importer->createAssetInstance(uid));

    if (!importer->import(sourcePath, asset.get()))
    {
        DEBUG_ERROR("[ModuleAssets] Import failed for '%s'.", sourcePath.string().c_str());
        if (!isReimport)
        {
            uid = INVALID_UID;
        }
        return;
    }

    if (!persistAsset(asset.get(), importer, uid, sourcePath))
    {
        if (!isReimport)
        {
            uid = INVALID_UID;
        }
    }
}

bool ModuleAssets::save(const Asset& asset, const std::filesystem::path& path)
{
    std::filesystem::path targetPath = path;

    if (targetPath.empty())
    {
        const Metadata* meta = m_registry->getMetadata(asset.getId());
        if (meta && !meta->sourcePath.empty())
        {
            targetPath = meta->sourcePath;
        }
    }

    if (targetPath.empty())
    {
        requestSave(asset);
        return false;
    }

    Importer* importer = findImporter(asset.getType());
    if (!importer)
    {
        DEBUG_ERROR("[ModuleAssets] No importer for type %u.", static_cast<unsigned>(asset.getType()));
        return false;
    }

    if (!importer->saveNative(&asset, targetPath))
    {
        DEBUG_ERROR("[ModuleAssets] saveToSource failed for '%s'.", targetPath.string().c_str());
        return false;
    }

    const UID uid = isValidUID(asset.getId()) ? asset.getId() : GenerateUID();
    return persistAsset(&asset, importer, uid, targetPath);
}

bool ModuleAssets::persistAsset(const Asset* asset, Importer* importer,
    const UID& uid, const std::filesystem::path& sourcePath)
{
    Metadata meta;
    const Metadata* existing = m_registry->getMetadata(uid);
    if (existing)
    {
        meta = *existing;
    }
    else
    {
        meta.uid = uid;
        meta.type = asset->getType();
        meta.sourcePath = sourcePath;
    }

    auto it = m_pendingDependencies.find(uid);
    if (it != m_pendingDependencies.end())
    {
        meta.m_dependencies = std::move(it->second);
        m_pendingDependencies.erase(it);
    }

    meta.contentHash = computeMD5(sourcePath);
    std::filesystem::path metaPath;
    meta.getMetadataPath(metaPath);

    if (!saveMetaFile(meta, metaPath))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write metadata for '%s'.", sourcePath.string().c_str());
        return false;
    }

    m_registry->registerAsset(meta);

    uint8_t* rawBuffer = nullptr;
    const uint64_t size = importer->save(asset, &rawBuffer);
    std::unique_ptr<uint8_t[]> buffer(rawBuffer);

    if (!FileIO::write(meta.getBinaryPath(), buffer.get(), static_cast<size_t>(size)))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write binary for '%s'.", sourcePath.string().c_str());
        m_registry->remove(uid);
        return false;
    }

    return true;
}



void ModuleAssets::refresh()
{
    std::string rootStr = ASSETS_FOLDER;
    if (!rootStr.empty() && (rootStr.back() == '/' || rootStr.back() == '\\'))
    {
        rootStr.pop_back();
    }

    const std::filesystem::path root = rootStr;

    std::vector<ImportRequest> pending = m_scanner->scan(root);
    for (ImportRequest& req : pending)
    {
        importAsset(req.sourcePath, req.existingUID);

    }
    m_contentRegistry->rebuild(root);
}

UID ModuleAssets::findUID(const std::filesystem::path& sourcePath) const
{
    return m_registry->findByPath(sourcePath);
}

bool ModuleAssets::isLoaded(const UID& id)
{
    return m_assets.contains(id);
}

void ModuleAssets::unload(const UID& id)
{
    m_assets.remove(id);
}

std::shared_ptr<FileEntry> ModuleAssets::getRoot() const
{
    return m_contentRegistry->getRoot();
}

std::shared_ptr<FileEntry> ModuleAssets::getEntry(const std::filesystem::path& path) const
{
    return m_contentRegistry->getEntry(path);
}

std::shared_ptr<Asset> ModuleAssets::loadAsset(const Metadata* metadata)
{
    Importer* importer = findImporter(metadata->type);
    if (!importer)
    {
        DEBUG_ERROR("[ModuleAssets] No importer for asset type %u (UID '%s').", static_cast<unsigned>(metadata->type), metadata->uid);
        return nullptr;
    }

    std::shared_ptr<Asset> asset(importer->createAssetInstance(metadata->uid));

    const std::vector<uint8_t> buffer = FileIO::read(metadata->getBinaryPath());
    if (buffer.empty())
    {
        DEBUG_ERROR("[ModuleAssets] Binary missing or empty for UID '%s'.", metadata->uid);
        return nullptr;
    }

    importer->load(buffer.data(), asset.get());
    m_assets.insert(metadata->uid, asset);

    return asset;
}

bool ModuleAssets::saveMetaFile(const Metadata& meta, const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("uid",         meta.uid, alloc);
    doc.AddMember("contentHash", rapidjson::Value(meta.contentHash.c_str(), alloc), alloc);
    doc.AddMember("type",        rapidjson::Value(static_cast<uint32_t>(meta.type)), alloc);
    doc.AddMember("sourcePath",  rapidjson::Value(meta.sourcePath.string().c_str(), alloc), alloc);

    // Write the dependency list so the scanner can re-register sub-assets on
    // the next startup without re-importing the parent source file.
    if (!meta.m_dependencies.empty())
    {
        rapidjson::Value deps(rapidjson::kArrayType);
        for (const DependencyRecord& dep : meta.m_dependencies)
        {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("uid",         dep.uid, alloc);
            entry.AddMember("contentHash", rapidjson::Value(dep.contentHash.c_str(), alloc), alloc);
            entry.AddMember("type",        rapidjson::Value(static_cast<uint32_t>(dep.type)), alloc);
            deps.PushBack(entry, alloc);
        }
        doc.AddMember("dependencies", deps, alloc);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(metaPath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[AssetMetadata] Could not open '%s' for writing.", metaPath.string().c_str());
        return false;
    }

    file << buffer.GetString();
    if (!file)
    {
        DEBUG_ERROR("[AssetMetadata] Failed to write '%s'.", metaPath.string().c_str());
        return false;
    }

    return true;
}

bool ModuleAssets::loadMetaFile(const std::filesystem::path& metaPath, Metadata& outMeta)
{
    const std::string pathStr = metaPath.string();
    FILE* fp = std::fopen(pathStr.c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[AssetMetadata] Could not open '%s'.", pathStr.c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[AssetMetadata] JSON parse error in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("uid") && doc["uid"].IsUint64())
        outMeta.uid = doc["uid"].GetUint64();
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing 'uid' in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("contentHash") && doc["contentHash"].IsString())
        outMeta.contentHash = doc["contentHash"].GetString();
    else
        outMeta.contentHash = INVALID_ASSET_ID;     // will trigger re-import

    if (doc.HasMember("type") && doc["type"].IsNumber())
        outMeta.type = static_cast<AssetType>(doc["type"].GetUint());
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing 'type' in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("sourcePath") && doc["sourcePath"].IsString())
        outMeta.sourcePath = doc["sourcePath"].GetString();

    outMeta.m_dependencies.clear();
    if (doc.HasMember("dependencies") && doc["dependencies"].IsArray())
    {
        const auto& deps = doc["dependencies"];
        outMeta.m_dependencies.reserve(deps.Size());
        for (rapidjson::SizeType i = 0; i < deps.Size(); ++i)
        {
            const auto& entry = deps[i];
            if (!entry.HasMember("uid")  || !entry["uid"].IsUint64())  continue;
            if (!entry.HasMember("type") || !entry["type"].IsNumber()) continue;

            DependencyRecord rec;
            rec.uid  = entry["uid"].GetUint64();
            rec.type = static_cast<AssetType>(entry["type"].GetUint());

            if (entry.HasMember("contentHash") && entry["contentHash"].IsString())
            {
                rec.contentHash = entry["contentHash"].GetString();
            }

            outMeta.m_dependencies.push_back(std::move(rec));
        }
    }

    return true;
}

void ModuleAssets::registerSubAsset(const Metadata& meta,
    const UID& parentUID,
    uint8_t* binaryData, size_t binarySize)
{
    Metadata subMeta       = meta;
    subMeta.m_isSubAsset   = true;

    if (binaryData && binarySize > 0)
    {
        const std::vector<int8_t> hashInput( reinterpret_cast<const int8_t*>(binaryData), reinterpret_cast<const int8_t*>(binaryData) + binarySize);

        subMeta.contentHash = to_hex_string(computeMD5(hashInput));
    }

    if (!isValidAsset(subMeta.contentHash))
    {
        DEBUG_ERROR("[ModuleAssets] Cannot register sub-asset (UID '%s'): binary data is null or empty.",
            subMeta.uid);
        return;
    }

    // Write the binary to Library/ — caller still owns the buffer.
    if (binaryData && binarySize > 0)
    {
        if (!FileIO::write(subMeta.getBinaryPath(), binaryData, binarySize))
        {
            DEBUG_ERROR("[ModuleAssets] Failed to write sub-asset binary (UID '%s').", subMeta.uid);
            return;
        }
    }

    m_registry->registerAsset(subMeta);

    if (isValidUID(parentUID))
    {
        DependencyRecord dep;
        dep.uid         = subMeta.uid;
        dep.contentHash = subMeta.contentHash;
        dep.type        = subMeta.type;
        m_pendingDependencies[parentUID].push_back(dep);
    }
}

bool ModuleAssets::saveAnimationStateMachine(const std::shared_ptr<AnimationStateMachineAsset>& asset)
{
    if (!asset)
    {
        DEBUG_ERROR("[ModuleAssets] saveAnimationStateMachine called with null asset.");
        return false;
    }
    if (!m_importerAnimationStateMachine)
    {
        DEBUG_ERROR("[ModuleAssets] AnimationStateMachine importer is not initialized.");
        return false;
    }

    if (!saveAnimationStateMachineSource(asset))
        return false;

    const Metadata* meta = m_registry->getMetadata(asset->getId());
    if (!meta)
    {
        DEBUG_ERROR("[ModuleAssets] No metadata found for AnimationStateMachine '%s'.", asset->getId());
        return false;
    }

    UID uid = asset->getId();
    importAsset(meta->sourcePath, uid);
    m_assets.remove(asset->getId());

    return isValidUID(uid);
}

bool ModuleAssets::saveAnimationStateMachineSource(const std::shared_ptr<AnimationStateMachineAsset>& asset)
{
    if (!asset) return false;

    const Metadata* meta = m_registry->getMetadata(asset->getId());
    if (!meta)
    {
        DEBUG_ERROR("[ModuleAssets] No metadata found for AnimationStateMachine '%s'.", asset->getId());
        return false;
    }

    Metadata updatedMeta = *meta;

    // HOTFIX: old state machines have no sourcePath — create one automatically.
    if (updatedMeta.sourcePath.empty())
    {
        std::filesystem::path dir = std::filesystem::path(ASSETS_FOLDER) / "StateMachines";
        std::filesystem::create_directories(dir);

        std::string fileName = asset->getName();
        if (fileName.empty()) fileName = asset->getId();

        // Very simple sanitisation.
        for (char& c : fileName)
        {
            if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*'
             || c == '?' || c == '"' || c == '<'  || c == '>' || c == '|')
                c = '_';
        }

        updatedMeta.sourcePath = dir / (fileName + ".statemachine");

        std::filesystem::path metaPath = updatedMeta.sourcePath;
        metaPath += METADATA_EXTENSION;

        if (!saveMetaFile(updatedMeta, metaPath))
        {
            DEBUG_ERROR("[ModuleAssets] Failed to create metadata for '%s'.",
                updatedMeta.sourcePath.string().c_str());
            return false;
        }

        m_registry->registerAsset(updatedMeta);
    }

    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("name",         rapidjson::Value(asset->getName().c_str(), alloc), alloc);
    doc.AddMember("defaultState", rapidjson::Value(asset->getDefaultStateName().c_str(), alloc), alloc);

    {
        rapidjson::Value clips(rapidjson::kArrayType);
        for (const AnimationStateMachineClip& clip : asset->getClips())
        {
            rapidjson::Value clipJson(rapidjson::kObjectType);
            clipJson.AddMember("name",         rapidjson::Value(clip.name.c_str(), alloc), alloc);
            clipJson.AddMember("animationUID", clip.animationUID, alloc);
            clipJson.AddMember("loop",         clip.loop, alloc);
            clips.PushBack(clipJson, alloc);
        }
        doc.AddMember("clips", clips, alloc);
    }

    {
        rapidjson::Value states(rapidjson::kArrayType);
        for (const AnimationStateMachineState& state : asset->getStates())
        {
            rapidjson::Value stateJson(rapidjson::kObjectType);
            stateJson.AddMember("name",                 rapidjson::Value(state.name.c_str(), alloc), alloc);
            stateJson.AddMember("clipName",             rapidjson::Value(state.clipName.c_str(), alloc), alloc);
            stateJson.AddMember("speed",                state.speed, alloc);
            stateJson.AddMember("behaviourScriptName",  rapidjson::Value(state.behaviourScriptName.c_str(), alloc), alloc);
            stateJson.AddMember("behaviourFieldsJson",  rapidjson::Value(state.behaviourFieldsJson.c_str(), alloc), alloc);
            stateJson.AddMember("overrideLoop",         state.overrideLoop, alloc);
            stateJson.AddMember("loop",                 state.loop, alloc);
            states.PushBack(stateJson, alloc);
        }
        doc.AddMember("states", states, alloc);
    }

    {
        rapidjson::Value transitions(rapidjson::kArrayType);
        for (const AnimationStateMachineTransition& transition : asset->getTransitions())
        {
            rapidjson::Value transitionJson(rapidjson::kObjectType);
            transitionJson.AddMember("sourceStateName",  rapidjson::Value(transition.sourceStateName.c_str(), alloc), alloc);
            transitionJson.AddMember("targetStateName",  rapidjson::Value(transition.targetStateName.c_str(), alloc), alloc);
            transitionJson.AddMember("triggerName",      rapidjson::Value(transition.triggerName.c_str(), alloc), alloc);
            transitionJson.AddMember("blendTimeSeconds", transition.blendTimeSeconds, alloc);
            transitions.PushBack(transitionJson, alloc);
        }
        doc.AddMember("transitions", transitions, alloc);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(updatedMeta.sourcePath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[ModuleAssets] Could not open '%s' for writing.", updatedMeta.sourcePath.string().c_str());
        return false;
    }

    file << buffer.GetString();
    return file.good();
}

void ModuleAssets::flushDependencies(const UID& parentUID,
    const std::filesystem::path& parentSourcePath,
    AssetType parentType)
{
    auto it = m_pendingDependencies.find(parentUID);
    if (it == m_pendingDependencies.end()) return;

    // Upsert the parent's registry entry with the collected dependency list.
    Metadata parentMeta;
    const Metadata* existing = m_registry->getMetadata(parentUID);
    if (existing)
    {
        parentMeta = *existing;
    }
    else
    {
        parentMeta.uid        = parentUID;
        parentMeta.type       = parentType;
        parentMeta.sourcePath = parentSourcePath;
    }

    parentMeta.m_dependencies = std::move(it->second);
    m_pendingDependencies.erase(it);

    m_registry->registerAsset(parentMeta);
}

bool ModuleAssets::savePrefab(GameObject* go, const fs::path& savePath)
{
    if (!go || savePath.empty()) return false;

    Document doc;
    const std::string json = PrefabSerializer::buildPrefabJSON(go, savePath);
    doc.Parse(json.c_str());

    if (!PrefabSerializer::writeDocument(doc, savePath)) return false;

    go->GetPrefabInfo().m_sourcePath = savePath;

    // Evict the stale cached asset first.
    UID uid = findUID(savePath);
    if (isValidUID(uid))
    {
        unload(uid);
    }

    importAsset(savePath, uid);

    if (auto asset = load<PrefabAsset>(uid))
    {
        asset->getData().m_json = json;
    }

    return true;
}

bool ModuleAssets::applyPrefab(const GameObject* go)
{
    const PrefabInfo& info = go->GetPrefabInfo();
    if (!info.isInstance()) return false;

    if (!savePrefab(const_cast<GameObject*>(go), info.m_sourcePath))
        return false;

    // Propagate the saved prefab to all live instances in the scene.
    Scene* scene = app->getModuleScene()->getScene();
    if (!scene) return true;

    const std::filesystem::path& prefabPath = info.m_sourcePath;
    for (GameObject* instance : scene->getAllGameObjects())
    {
        if (!instance) continue;
        if (!instance->GetPrefabInfo().isInstance()) continue;
        if (instance->GetPrefabInfo().m_sourcePath != prefabPath) continue;
        if (instance == go) continue;

        revertPrefab(instance, scene);
    }

    scene->markDirty();
    return true;
}

bool ModuleAssets::revertPrefab(GameObject* go, Scene* scene)
{
    PrefabInfo& info = go->GetPrefabInfo();
    if (!info.isInstance()) return false;

    Document doc;
    bool loaded = false;
    if (isValidUID(info.m_assetUID))
    {
        auto asset = load<PrefabAsset>(info.m_assetUID);
        if (asset && !asset->getJSON().empty())
        {
            doc.Parse(asset->getJSON().c_str());
            loaded = !doc.HasParseError() && doc.HasMember("GameObject");
        }
    }
    if (!loaded)
        loaded = PrefabSerializer::loadDocument(info.m_sourcePath, doc);
    if (!loaded) return false;

    const Value& goNode = doc["GameObject"];
    const PrefabOverrideRecord savedOverrides = info.m_overrides;
    const int transformType = static_cast<int>(ComponentType::TRANSFORM);

    if (goNode.HasMember("Transform") && goNode["Transform"].IsObject())
    {
        Transform* tf = go->GetTransform();
        const Value& tfNode = goNode["Transform"];

        auto oit = savedOverrides.m_modifiedProperties.find(transformType);
        const auto* overrideSet = (oit != savedOverrides.m_modifiedProperties.end())
            ? &oit->second : nullptr;
        auto isOverridden = [&](const char* prop)
            { return overrideSet && overrideSet->count(prop) > 0; };

        if (!isOverridden("position") && tfNode.HasMember("position") && tfNode["position"].IsArray())
        {
            const auto& p = tfNode["position"];
            tf->setPosition(Vector3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat()));
        }
        if (!isOverridden("rotation") && tfNode.HasMember("rotation") && tfNode["rotation"].IsArray())
        {
            const auto& r = tfNode["rotation"];
            tf->setRotation(Quaternion(r[0].GetFloat(), r[1].GetFloat(), r[2].GetFloat(), r[3].GetFloat()));
        }
        if (!isOverridden("scale") && tfNode.HasMember("scale") && tfNode["scale"].IsArray())
        {
            const auto& s = tfNode["scale"];
            tf->setScale(Vector3(s[0].GetFloat(), s[1].GetFloat(), s[2].GetFloat()));
        }
        tf->markDirty();
    }

    if (goNode.HasMember("Components") && goNode["Components"].IsArray())
    {
        for (SizeType i = 0; i < goNode["Components"].Size(); ++i)
        {
            const Value& cn = goNode["Components"][i];
            if (!cn.HasMember("Type") || !cn.HasMember("Data")) continue;

            const int ct = cn["Type"].GetInt();
            auto oit = savedOverrides.m_modifiedProperties.find(ct);
            if (oit != savedOverrides.m_modifiedProperties.end()
             && oit->second.count("properties") > 0)
                continue;

            Component* comp = go->GetComponent(static_cast<ComponentType>(ct));
            if (comp) comp->deserializeJSON(cn["Data"]);
        }
    }

    info.m_overrides = savedOverrides;
    return true;
}

bool ModuleAssets::createVariant(const fs::path& src, const fs::path& dst)
{
    if (src.empty() || dst.empty()) return false;

    Document doc;
    if (!PrefabSerializer::readDocument(src, doc)) return false;
    auto& alloc = doc.GetAllocator();

    auto setOrAdd = [&](const char* key, const std::string& value)
    {
        if (doc.HasMember(key))
            doc[key].SetString(value.c_str(), alloc);
        else
            doc.AddMember(Value(key, alloc), Value(value.c_str(), alloc), alloc);
    };

    setOrAdd("SourcePath", dst.string());
    setOrAdd("Name",       dst.stem().string());
    setOrAdd("VariantOf",  src.string());

    return PrefabSerializer::writeDocument(doc, dst);
}

GameObject* ModuleAssets::spawnPrefab(const PrefabAsset& asset, Scene* scene)
{
    if (!scene || asset.getJSON().empty()) return nullptr;

    Document doc;
    doc.Parse(asset.getJSON().c_str());
    if (doc.HasParseError() || !doc.HasMember("GameObject") || !doc["GameObject"].IsObject())
    {
        DEBUG_ERROR("[ModuleAssets] Malformed JSON in prefab asset '%s'.", asset.getId());
        return nullptr;
    }

    GameObject* go = PrefabSerializer::deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    const auto& data = asset.getData();
    PrefabInfo& info = go->GetPrefabInfo();
    info.m_sourcePath = data.m_sourcePath;
    info.m_assetUID   = data.m_assetUID;

    return go;
}

GameObject* ModuleAssets::spawnPrefab(const fs::path& sourcePath, Scene* scene)
{
    if (!scene || sourcePath.empty()) return nullptr;

    auto asset = loadAtPath<PrefabAsset>(sourcePath);
    if (asset) return spawnPrefab(*asset, scene);

    Document doc;
    if (!PrefabSerializer::loadDocument(sourcePath, doc)) return nullptr;

    GameObject* go = PrefabSerializer::deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    go->GetPrefabInfo().m_sourcePath = sourcePath;
    return go;
}

void ModuleAssets::requestSave(const Asset& asset)
{
    if (m_dialogRunning.load()) return;

    m_pendingAsset = &asset;
    m_pendingAssetType = asset.getType();
    m_pendingIsSave = true;
    m_dialogCallback = nullptr;

    { std::lock_guard lock(m_dialogResultMutex); m_dialogResult.reset(); }
    m_dialogRunning.store(true);

    std::thread([this]()
        {
            const AssetDialogFilter filter = getDialogFilter(m_pendingAssetType);
            auto result = saveAs(filter.filterSpec, filter.defaultExtension, "Save Asset", ASSETS_FOLDER);
            { std::lock_guard lock(m_dialogResultMutex); m_dialogResult = std::move(result); }
            m_dialogRunning.store(false);
        }).detach();
}



void ModuleAssets::flushDialogRequests()
{
    if (m_dialogRunning.load()) return;

    std::optional<std::filesystem::path> result;
    {
        std::lock_guard lock(m_dialogResultMutex);
        if (!m_dialogResult.has_value()) return;
        result = std::move(m_dialogResult);
        m_dialogResult.reset();
    }

    if (!result.has_value())
    {
        m_pendingAsset = nullptr;
        return;
    }

    if (m_pendingIsSave && m_pendingAsset)
    {
        save(*m_pendingAsset, *result);
        m_pendingAsset = nullptr;
    }
    else if (!m_pendingIsSave && m_dialogCallback)
    {
        m_dialogCallback(*result);
        m_dialogCallback = nullptr;
    }
}
