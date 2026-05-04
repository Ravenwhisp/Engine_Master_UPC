#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "Importer.h"
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
#include <algorithm>
#include <cctype>

#include <filesystem>
#include <FileIO.h>

using namespace rapidjson;
namespace fs = std::filesystem;

namespace
{
    std::string ToLowerString(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return value;
    }

    std::string SanitizeAssetFileName(std::string fileName)
    {
        for (char& c : fileName)
        {
            if (c == ' ' || c == '/' || c == '\\' || c == ':' ||
                c == '*' || c == '?' || c == '"' || c == '<' ||
                c == '>' || c == '|')
            {
                c = '_';
            }
        }

        return fileName;
    }
}

bool ModuleAssets::init()
{
    m_registry = std::make_unique<AssetRegistry>();
    m_importerRegistry = std::make_unique<ImporterRegistry>();
    
    m_importerRegistry->registerImporter(std::make_unique<ImporterTexture>());

    {
        auto mesh = std::make_unique<ImporterMesh>();
        m_importerMesh = mesh.get();
        m_importerRegistry->registerImporter(std::move(mesh));
    }
    {
        auto mat = std::make_unique<ImporterMaterial>();
        m_importerMaterial = mat.get();
        m_importerRegistry->registerImporter(std::move(mat));
    }
    {
        auto prefab = std::make_unique<ImporterPrefab>();
        m_importerPrefab = prefab.get();
        m_importerRegistry->registerImporter(std::move(prefab));
    }

    {
        auto anim = std::make_unique<ImporterAnimation>();
        m_importerAnimation = anim.get();
        m_importerRegistry->registerImporter(std::move(anim));
    }

    {
        auto skin = std::make_unique<ImporterSkin>();
        m_importerSkin = skin.get();
        m_importerRegistry->registerImporter(std::move(skin));
    }

    {
        auto animStateMachine = std::make_unique<ImporterAnimationStateMachine>();
        m_importerAnimationStateMachine = animStateMachine.get();
        m_importerRegistry->registerImporter(std::move(animStateMachine));
    }

    // GLTF importer holds references to the three importers above so it can
    // delegate sub-asset serialisation without duplicating binary format logic.
    m_importerRegistry->registerImporter(
        std::make_unique<ImporterGltf>(
            *m_importerMesh,
            *m_importerMaterial,
            *m_importerPrefab,
            *m_importerAnimation,
            *m_importerSkin,
            *m_importerAnimationStateMachine));

    m_importerRegistry->registerImporter(std::make_unique<ImporterFont>());

    m_scanner = std::make_unique<AssetScanner>(m_registry.get(), m_importerRegistry.get());
    m_contentRegistry = std::make_unique<ContentRegistry>(m_registry.get());

    refresh();

    return true;
}

bool ModuleAssets::cleanUp()
{
    m_assets.clear();
    return true;
}

bool ModuleAssets::canImport(const std::filesystem::path& sourcePath) const
{
    return m_importerRegistry->findImporter(sourcePath) != nullptr;
}

void ModuleAssets::importAsset(const std::filesystem::path& sourcePath, MD5Hash& uid)
{
    Importer* importer = m_importerRegistry->findImporter(sourcePath);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] No importer found for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    if (!isValidAsset(uid))
    {
        if (sourcePath.extension() == ".statemachine")
        {
            uid = computeStableUIDFromAssetPath(sourcePath);
        }
        else
        {
            uid = computeMD5(sourcePath);
        }
    }

    // Scoped to this function — not cached, not shared.
    std::unique_ptr<Asset> asset(importer->createAssetInstance(uid));

    if (!importer->import(sourcePath, asset.get()))
    {
        DEBUG_ERROR("[ModuleAssets] Import failed for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    // Write the .metadata sidecar alongside the source file.
    Metadata meta;
    meta.uid = uid;
    meta.type = asset->getType();
    meta.sourcePath = sourcePath;
    
    auto it = m_pendingDependencies.find(uid);
    if (it != m_pendingDependencies.end()) {
        meta.m_dependencies = std::move(it->second);
        m_pendingDependencies.erase(it);
    }

    std::filesystem::path metaPath = sourcePath;
    metaPath += METADATA_EXTENSION;

    if (!saveMetaFile(meta, metaPath))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write metadata for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    m_registry->registerAsset(meta);

    // Serialise the processed binary into the library folder.
    uint8_t* rawBuffer = nullptr;
    const uint64_t size = importer->save(asset.get(), &rawBuffer);
    std::unique_ptr<uint8_t[]> buffer(rawBuffer);

    if (!FileIO::write(meta.getBinaryPath(), buffer.get(), static_cast<size_t>(size)))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write binary for '%s'.", sourcePath.string().c_str());
        // Metadata was already written — roll back the in-memory store entry
        // so it doesn't reference a binary that doesn't exist.
        m_registry->remove(uid);
        uid = INVALID_ASSET_ID;
        return;
    }

    return;
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


MD5Hash ModuleAssets::findUID(const std::filesystem::path& sourcePath) const
{
    return m_registry->findByPath(sourcePath);
}

MD5Hash ModuleAssets::computeStableUIDFromAssetPath(const std::filesystem::path& sourcePath) const
{
    fs::path normalizedSource = sourcePath.lexically_normal();
    fs::path assetsRoot = fs::path(ASSETS_FOLDER).lexically_normal();

    fs::path relativePath;

    std::error_code ec;
    relativePath = fs::relative(normalizedSource, assetsRoot, ec);

    const std::string relativePathString = relativePath.generic_string();

    if (ec || relativePath.empty() || relativePathString.rfind("../", 0) == 0 || relativePathString == "..")
    {
        relativePath = normalizedSource;
    }
    else
    {
        relativePath = assetsRoot.filename() / relativePath;
    }

    std::string stablePath = relativePath.generic_string();
    stablePath = ToLowerString(stablePath);

    return computeMD5(stablePath);
}

std::filesystem::path ModuleAssets::getDefaultStateMachineSourcePath(const std::filesystem::path& modelSourcePath) const
{
    std::string fileName = modelSourcePath.stem().string() + "_StateMachine";
    fileName = SanitizeAssetFileName(fileName);

    return fs::path(ASSETS_FOLDER) / "StateMachines" / (fileName + ".statemachine");
}

bool ModuleAssets::isLoaded(const MD5Hash& id)
{
    return m_assets.contains(id);
}

void ModuleAssets::unload(const MD5Hash& id)
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
    Importer* importer = m_importerRegistry->findImporter(metadata->type);
    if (!importer)
    {
        DEBUG_ERROR("[ModuleAssets] No importer for asset type %u (UID %llu).", static_cast<unsigned>(metadata->type), metadata->uid);
        return nullptr;
    }

    std::shared_ptr<Asset> asset(importer->createAssetInstance(metadata->uid));

    const std::vector<uint8_t> buffer = FileIO::read(metadata->getBinaryPath());
    if (buffer.empty())
    {
        DEBUG_ERROR("[ModuleAssets] Binary missing or empty for UID %llu.", metadata->uid);
        return nullptr;
    }

    importer->load(buffer.data(), asset.get());
    m_assets.insert(metadata->uid, asset);

    return asset;
}

bool ModuleAssets::saveMetaFile(const Metadata& meta,
    const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("uid", rapidjson::Value(meta.uid.c_str(), alloc), alloc);
    doc.AddMember("type", rapidjson::Value(static_cast<uint32_t>(meta.type)), alloc);
    doc.AddMember("sourcePath", rapidjson::Value(meta.sourcePath.string().c_str(), alloc), alloc);

    // Write the dependency list so the scanner can re-register sub-assets on
    // the next startup without re-importing the parent source file.
    if (!meta.m_dependencies.empty())
    {
        rapidjson::Value deps(rapidjson::kArrayType);
        for (const DependencyRecord& dep : meta.m_dependencies)
        {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("uid", rapidjson::Value(dep.uid.c_str(), alloc), alloc);
            entry.AddMember("type", rapidjson::Value(static_cast<uint32_t>(dep.type)), alloc);
            entry.AddMember("isSubAsset", dep.isSubAsset, alloc);
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
        DEBUG_ERROR("[AssetMetadata] Could not open '%s' for writing.",
            metaPath.string().c_str());
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

    if (doc.HasMember("uid") && doc["uid"].IsString())
    {
        outMeta.uid = doc["uid"].GetString();
    }
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing 'uid' in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("type") && doc["type"].IsNumber())
    {
        outMeta.type = static_cast<AssetType>(doc["type"].GetUint());
    }
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing 'type' in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("sourcePath") && doc["sourcePath"].IsString())
    {
        outMeta.sourcePath = doc["sourcePath"].GetString();
    }


    // Restore the dependency list.
    outMeta.m_dependencies.clear();
    if (doc.HasMember("dependencies") && doc["dependencies"].IsArray())
    {
        const auto& deps = doc["dependencies"];
        outMeta.m_dependencies.reserve(deps.Size());
        for (rapidjson::SizeType i = 0; i < deps.Size(); ++i)
        {
            const auto& entry = deps[i];
            if (!entry.HasMember("uid") || !entry["uid"].IsString())  continue;
            if (!entry.HasMember("type") || !entry["type"].IsNumber()) continue;

            DependencyRecord rec;
            rec.uid = entry["uid"].GetString();
            rec.type = static_cast<AssetType>(entry["type"].GetUint());

            if (entry.HasMember("isSubAsset") && entry["isSubAsset"].IsBool())
            {
                rec.isSubAsset = entry["isSubAsset"].GetBool();
            }
            else
            {
                rec.isSubAsset = true;
            }

            outMeta.m_dependencies.push_back(std::move(rec));
        }
    }

    return true;
}

void ModuleAssets::registerSubAsset(const Metadata& meta,
    const MD5Hash& parentUID,
    uint8_t* binaryData, size_t binarySize)
{
    Metadata subMeta = meta;
    subMeta.m_isSubAsset = true;

    // Write the binary to Library/ — caller still owns the buffer.
    if (binaryData && binarySize > 0)
    {
        if (!FileIO::write(subMeta.getBinaryPath(), binaryData, binarySize))
        {
            DEBUG_ERROR("[ModuleAssets] Failed to write sub-asset binary '%s'.", subMeta.uid.c_str());
            return;
        }
    }

    m_registry->registerAsset(subMeta);

    if (isValidAsset(parentUID))
    {
        DependencyRecord dep;
        dep.uid = subMeta.uid;
        dep.type = subMeta.type;
        dep.isSubAsset = true;
        m_pendingDependencies[parentUID].push_back(dep);
    }
}

void ModuleAssets::registerDependency(const MD5Hash& parentUID, const MD5Hash& dependencyUID, AssetType dependencyType)
{
    if (!isValidAsset(parentUID) || !isValidAsset(dependencyUID))
        return;

    DependencyRecord dep;
    dep.uid = dependencyUID;
    dep.type = dependencyType;
    dep.isSubAsset = false;


    m_pendingDependencies[parentUID].push_back(dep);
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
    {
        return false;
    }

    const Metadata* meta = m_registry->getMetadata(asset->getId());
    if (!meta)
    {
        DEBUG_ERROR("[ModuleAssets] No metadata found for AnimationStateMachine '%s'.", asset->getId().c_str());
        return false;
    }

    MD5Hash uid = asset->getId();
    importAsset(meta->sourcePath, uid);
    m_assets.remove(asset->getId());

    return uid != INVALID_ASSET_ID;
}

bool ModuleAssets::saveAnimationStateMachineSource(const std::shared_ptr<AnimationStateMachineAsset>& asset)
{
    if (!asset)
    {
        return false;
    }

    const Metadata* meta = m_registry->getMetadata(asset->getId());
    if (!meta)
    {
        DEBUG_ERROR("[ModuleAssets] No metadata found for AnimationStateMachine '%s'.", asset->getId().c_str());
        return false;
    }

    Metadata updatedMeta = *meta;

    // HOTFIX: old state machines have no sourcePath. Create one automatically.
    if (updatedMeta.sourcePath.empty())
    {
        std::filesystem::path dir = std::filesystem::path(ASSETS_FOLDER) / "StateMachines";
        std::filesystem::create_directories(dir);

        std::string fileName = asset->getName();
        if (fileName.empty())
        {
            fileName = asset->getId();
        }

        // very simple sanitization
        for (char& c : fileName)
        {
            if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            {
                c = '_';
            }
        }

        updatedMeta.sourcePath = dir / (fileName + ".statemachine");

        std::filesystem::path metaPath = updatedMeta.sourcePath;
        metaPath += METADATA_EXTENSION;

        if (!saveMetaFile(updatedMeta, metaPath))
        {
            DEBUG_ERROR("[ModuleAssets] Failed to create metadata for '%s'.", updatedMeta.sourcePath.string().c_str());
            return false;
        }

        m_registry->registerAsset(updatedMeta);
    }

    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("name", rapidjson::Value(asset->getName().c_str(), alloc), alloc);
    doc.AddMember("defaultState", rapidjson::Value(asset->getDefaultStateName().c_str(), alloc), alloc);

    {
        rapidjson::Value clips(rapidjson::kArrayType);
        for (const AnimationStateMachineClip& clip : asset->getClips())
        {
            rapidjson::Value clipJson(rapidjson::kObjectType);
            clipJson.AddMember("name", rapidjson::Value(clip.name.c_str(), alloc), alloc);
            clipJson.AddMember("animationUID", rapidjson::Value(clip.animationUID.c_str(), alloc), alloc);
            clipJson.AddMember("loop", clip.loop, alloc);
            clips.PushBack(clipJson, alloc);
        }
        doc.AddMember("clips", clips, alloc);
    }

    {
        rapidjson::Value states(rapidjson::kArrayType);
        for (const AnimationStateMachineState& state : asset->getStates())
        {
            rapidjson::Value stateJson(rapidjson::kObjectType);
            stateJson.AddMember("name", rapidjson::Value(state.name.c_str(), alloc), alloc);
            stateJson.AddMember("clipName", rapidjson::Value(state.clipName.c_str(), alloc), alloc);
            stateJson.AddMember("speed", state.speed, alloc);
            stateJson.AddMember("behaviourScriptName", rapidjson::Value(state.behaviourScriptName.c_str(), alloc), alloc);
            stateJson.AddMember("behaviourFieldsJson", rapidjson::Value(state.behaviourFieldsJson.c_str(), alloc), alloc);
            stateJson.AddMember("overrideLoop", state.overrideLoop, alloc);
            stateJson.AddMember("loop", state.loop, alloc);
            states.PushBack(stateJson, alloc);
        }
        doc.AddMember("states", states, alloc);
    }

    {
        rapidjson::Value transitions(rapidjson::kArrayType);
        for (const AnimationStateMachineTransition& transition : asset->getTransitions())
        {
            rapidjson::Value transitionJson(rapidjson::kObjectType);
            transitionJson.AddMember("sourceStateName", rapidjson::Value(transition.sourceStateName.c_str(), alloc), alloc);
            transitionJson.AddMember("targetStateName", rapidjson::Value(transition.targetStateName.c_str(), alloc), alloc);
            transitionJson.AddMember("triggerName", rapidjson::Value(transition.triggerName.c_str(), alloc), alloc);
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

void ModuleAssets::flushDependencies(const MD5Hash& parentUID,
    const std::filesystem::path& parentSourcePath,
    AssetType parentType)
{
    auto it = m_pendingDependencies.find(parentUID);
    if (it == m_pendingDependencies.end())
        return;

    // Upsert the parent's registry entry with the collected dependency list.
    Metadata parentMeta;
    const Metadata* existing = m_registry->getMetadata(parentUID);
    if (existing)
        parentMeta = *existing;
    else
    {
        parentMeta.uid = parentUID;
        parentMeta.type = parentType;
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
    MD5Hash uid = findUID(savePath);
    if (isValidAsset(uid))
    {
        unload(uid);
    }

    // Re-import so the Library/ binary is regenerated from the new JSON file.
    // This also re-registers metadata and updates the registry.
    importAsset(savePath, uid);

    // Now update the in-memory asset cache with the new JSON so that any
    // already-held shared_ptrs see the current data without a second disk read.
    if (auto asset = load<PrefabAsset>(uid))
    {
        asset->getData().m_json = json;
    }

    return true;
}

bool ModuleAssets::applyPrefab(const GameObject* go)
{
    const PrefabInstanceInfo& info = go->GetPrefabInfo();
    if (!info.isInstance()) return false;

    if (!savePrefab(const_cast<GameObject*>(go), info.m_sourcePath))
        return false;

    // Propagate the saved prefab to all live instances in the scene
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
    PrefabInstanceInfo& info = go->GetPrefabInfo();
    if (!info.isInstance()) return false;

    Document doc;
    bool loaded = false;
    if (isValidAsset(info.m_assetUID))
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
    // Sentinel key -1 holds GameObject-level property overrides (name, active, tag, layer)
    const int goLevelKey = -1;
    auto goOit = savedOverrides.m_modifiedProperties.find(goLevelKey);
    const auto* goOverrideSet = (goOit != savedOverrides.m_modifiedProperties.end())
        ? &goOit->second : nullptr;
    auto isGoOverridden = [&](const char* prop)
        { return goOverrideSet && goOverrideSet->count(prop) > 0; };

    if (!isGoOverridden("name") && goNode.HasMember("Name") && goNode["Name"].IsString())
        go->SetName(goNode["Name"].GetString());

    if (!isGoOverridden("active") && goNode.HasMember("Active") && goNode["Active"].IsBool())
        go->SetActive(goNode["Active"].GetBool());

    // Tag and Layer are not currently written by PrefabSerializer::serialiseNodeInto,
    // so we only restore them if the JSON actually has them.
    if (!isGoOverridden("tag") && goNode.HasMember("Tag") && goNode["Tag"].IsString())
        go->SetTag(StringToTag(goNode["Tag"].GetString()));

    if (!isGoOverridden("layer") && goNode.HasMember("Layer") && goNode["Layer"].IsString())
        go->SetLayer(StringToLayer(goNode["Layer"].GetString()));

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
        // Build an ordered list of all components per type, matching serialization order
        std::unordered_map<int, std::vector<Component*>> componentsByType;
        for (Component* comp : go->GetAllComponents())
        {
            componentsByType[static_cast<int>(comp->getType())].push_back(comp);
        }
        // Track how many of each type 
        std::unordered_map<int, size_t> typeIndex;

        for (SizeType i = 0; i < goNode["Components"].Size(); ++i)
        {
            const Value& cn = goNode["Components"][i];
            if (!cn.HasMember("Type") || !cn.HasMember("Data")) continue;

            const int ct = cn["Type"].GetInt();

            // Skip if this component type is overridden on this instance
            auto oit = savedOverrides.m_modifiedProperties.find(ct);
            if (oit != savedOverrides.m_modifiedProperties.end()
                && oit->second.count("properties") > 0)
                continue;

            auto& comps = componentsByType[ct];
            size_t& idx = typeIndex[ct];
            if (idx < comps.size())
            {
                comps[idx]->deserializeJSON(cn["Data"]);
                ++idx;
            }
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
    setOrAdd("Name", dst.stem().string());
    setOrAdd("VariantOf", src.string());

    return PrefabSerializer::writeDocument(doc, dst);
}

GameObject* ModuleAssets::spawnPrefab(const PrefabAsset& asset, Scene* scene)
{
    if (!scene || asset.getJSON().empty()) return nullptr;

    Document doc;
    doc.Parse(asset.getJSON().c_str());
    if (doc.HasParseError() || !doc.HasMember("GameObject") || !doc["GameObject"].IsObject())
    {
        DEBUG_ERROR("[ModuleAssets] Malformed JSON in prefab asset '%s'.", asset.getId().c_str());
        return nullptr;
    }

    GameObject* go = PrefabSerializer::deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    const auto& data = asset.getData();
    PrefabInstanceInfo& info = go->GetPrefabInfo();
    info.m_sourcePath = data.m_sourcePath;
    info.m_assetUID = data.m_assetUID;

    return go;
}

GameObject* ModuleAssets::spawnPrefab(const fs::path& sourcePath, Scene* scene)
{
    if (!scene || sourcePath.empty()) return nullptr;

    auto asset = loadAtPath<PrefabAsset>(sourcePath);
    if (asset)
        return spawnPrefab(*asset, scene);

    Document doc;
    if (!PrefabSerializer::loadDocument(sourcePath, doc)) return nullptr;

    GameObject* go = PrefabSerializer::deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    go->GetPrefabInfo().m_sourcePath = sourcePath;
    return go;
}