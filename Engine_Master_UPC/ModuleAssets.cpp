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

#include <filesystem>
#include <FileIO.h>
#include <AssetDialogFilter.h>
#include <FileDialog.h>
#include "FileDialogRequest.h"

using namespace rapidjson;
namespace fs = std::filesystem;
ModuleAssets::~ModuleAssets() = default;

bool ModuleAssets::init()
{
    m_registry = std::make_unique<AssetRegistry>();
    
	m_importers.push_back(m_importerTexture = new ImporterTexture());
    m_importers.push_back(m_importerMesh = new ImporterMesh());
    m_importers.push_back(m_importerMaterial = new ImporterMaterial());
    m_importers.push_back(m_importerPrefab = new ImporterPrefab());
    m_importers.push_back(m_importerAnimation = new ImporterAnimation());
    m_importers.push_back(m_importerSkin = new ImporterSkin());
    m_importers.push_back(m_importerAnimationStateMachine = new ImporterAnimationStateMachine());
    m_importers.push_back(m_importerGltf = new ImporterGltf(m_importerMesh, m_importerMaterial, m_importerPrefab, m_importerAnimation, m_importerSkin, m_importerAnimationStateMachine));
    m_importers.push_back(m_importerFont = new ImporterFont());

    m_scanner = std::make_unique<AssetScanner>(m_registry.get());
    m_contentRegistry = std::make_unique<ContentRegistry>(m_registry.get());

    refresh();

    return true;
}

bool ModuleAssets::cleanUp()
{
    m_assets.clear();
    for (auto it = m_importers.rbegin(); it != m_importers.rend(); ++it)
    {
        delete* it;
    }

    return true;
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

std::shared_ptr<FileEntry> ModuleAssets::getRoot() const
{
    return m_contentRegistry->getRoot();
}

std::shared_ptr<FileEntry> ModuleAssets::getEntry(const std::filesystem::path& path) const
{
    return m_contentRegistry->getEntry(path);
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

    // Write source file � only native importers support this.
    if (importer->canSaveToSource())
    {
        if (!importer->saveToSource(targetPath, &asset))
        {
            DEBUG_ERROR("[ModuleAssets] saveToSource failed for '%s'.", targetPath.string().c_str());
            return false;
        }
    }

    const UID uid = isValidUID(asset.getId()) ? asset.getId() : GenerateUID();
    return persistAsset(&asset, importer, uid, targetPath);
}

bool ModuleAssets::writeMetadata(const Metadata& meta, const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();
    if (!meta.toJson(doc))
    {
        DEBUG_ERROR("[ModuleAssets] Metadata::toJson failed for '%s'.", metaPath.string().c_str());
        return false;
    }

    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);

    std::ofstream file(metaPath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[ModuleAssets] Cannot open '%s' for writing.", metaPath.string().c_str());
        return false;
    }
    file << buf.GetString();
    return file.good();
}

bool ModuleAssets::loadMetadata(const std::filesystem::path& path, Metadata& out)
{
    const std::string pathStr = path.string();
    FILE* fp = std::fopen(pathStr.c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[ModuleAssets] Could not open '%s'.", pathStr.c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    rapidjson::Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[ModuleAssets] JSON parse error in '%s'.", pathStr.c_str());
        return false;
    }

    return out.fromJson(doc);
}

void ModuleAssets::registerSubAsset(const DependencyRecord& dep, UID parentFileId)
{
    if (!isValidUID(parentFileId))
    {
        DEBUG_WARN("[ModuleAssets] registerSubAsset called with invalid parentFileId.");
        return;
    }

    if (!isValidUID(dep.localId))
    {
        DEBUG_WARN("[ModuleAssets] registerSubAsset called with invalid dep.localId.");
        return;
    }

    m_pendingDependencies[parentFileId].push_back(dep);
}

#pragma region Importer

Importer* ModuleAssets::findImporter(const std::filesystem::path& filePath) const
{
    for (auto& importer : m_importers)
    {
        if (importer->canImport(filePath))
        {
            return importer;
        }
    }
    return nullptr;
}

Importer* ModuleAssets::findImporter(AssetType type) const
{
    for (auto& importer : m_importers)
    {
        if (importer->getAssetType() == type)
        {
            return importer;
        }
    }
    return nullptr;
}

#pragma endregion

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
        meta.fileId = uid;
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

    const std::filesystem::path metaPath = Metadata::toMetadataPath(sourcePath);
    if (!writeMetadata(meta, metaPath))
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


bool ModuleAssets::applyPrefab(GameObject* go)
{
    PrefabAsset prefab = PrefabAsset(GenerateUID(), go);
    save(prefab);

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
    PrefabInfo& info = go->GetPrefabInfo();
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
        DEBUG_ERROR("[ModuleAssets] Malformed JSON in prefab asset '%s'.", asset.getId());
        return nullptr;
    }

    GameObject* go = PrefabSerializer::deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    const auto& data = asset.getData();
    PrefabInfo& info = go->GetPrefabInfo();
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