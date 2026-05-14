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
#include "ImporterScene.h"
#include "MD5.h"

#include "AssetScanner.h"
#include "ContentRegistry.h"
#include "PrefabManager.h"

#include "PrefabSerializer.h"
#include "PrefabAsset.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "ModuleScene.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"
#include <fstream>

#include "Asset.h"
#include "AnimationStateMachineAsset.h"
#include "Metadata.h"
#include "UID.h"

#include <filesystem>
#include <FileIO.h>
#include <AssetDialogFilter.h>
#include <FileDialog.h>

using namespace rapidjson;
namespace fs = std::filesystem;

constexpr bool ASSETS_MY_DEBUG = false;
#define DEBUG_ASSETS(...) do { if constexpr (ASSETS_MY_DEBUG) { DEBUG_LOG(__VA_ARGS__); } } while (0)

double elapsedMs(
    const std::chrono::high_resolution_clock::time_point& begin,
    const std::chrono::high_resolution_clock::time_point& end)
{
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

ModuleAssets::ModuleAssets() = default;
ModuleAssets::~ModuleAssets() = default;

bool ModuleAssets::init()
{
    m_importers.push_back(m_importerTexture = new ImporterTexture());
    m_importers.push_back(m_importerMesh = new ImporterMesh());
    m_importers.push_back(m_importerMaterial = new ImporterMaterial());
    m_importers.push_back(m_importerPrefab = new ImporterPrefab());
    m_importers.push_back(m_importerAnimation = new ImporterAnimation());
    m_importers.push_back(m_importerSkin = new ImporterSkin());
    m_importers.push_back(m_importerAnimationStateMachine = new ImporterAnimationStateMachine());
    m_importers.push_back(m_importerGltf = new ImporterGltf(
    m_importerMesh, m_importerMaterial, m_importerPrefab,
    m_importerAnimation, m_importerSkin, m_importerAnimationStateMachine));
    m_importers.push_back(m_importerFont = new ImporterFont());
    m_importers.push_back(m_importerScene = new ImporterScene());

    m_scanner = std::make_unique<AssetScanner>();
    m_contentRegistry = std::make_unique<ContentRegistry>(this);
    m_prefabManager = std::make_unique<PrefabManager>(this);

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
        delete* it;
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

void ModuleAssets::importAsset(const std::filesystem::path& sourcePath, AssetReference& reference)
{
    Importer* importer = findImporter(sourcePath);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] No importer found for '%s'.", sourcePath.string().c_str());
        reference = AssetReference();
        return;
    }

    const bool isReimport = isValidUID(reference.m_uid);
    UID uid = isReimport ? reference.m_uid : GenerateUID();

    reference.m_uid = uid;
    reference.m_type = importer->getAssetType();
    reference.m_libId = INVALID_ASSET_ID;

    std::unique_ptr<Asset> asset(importer->createAssetInstance(reference));

    if (!importer->import(sourcePath, asset.get()))
    {
        DEBUG_ERROR("[ModuleAssets] Import failed for '%s'.", sourcePath.string().c_str());
        if (!isReimport) reference = AssetReference();
        return;
    }

    if (!persistAsset(asset.get(), importer, reference, sourcePath))
    {
        if (!isReimport) reference = AssetReference();
    }
}


bool ModuleAssets::save(Asset& asset, const std::filesystem::path& path)
{
    std::filesystem::path targetPath = path;

    if (targetPath.empty())
    {
        auto it = m_uidIndex.find(asset.getUID());
        if (it != m_uidIndex.end() && !it->second.sourcePath.empty())
        {
            targetPath = it->second.sourcePath;
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
        DEBUG_ERROR("[ModuleAssets] saveNative failed for '%s'.", targetPath.string().c_str());
        return false;
    }

    const UID uid = isValidUID(asset.getUID()) ? asset.getUID() : GenerateUID();
    AssetReference ref(uid, INVALID_ASSET_ID, asset.getType());
    return persistAsset(&asset, importer, ref, targetPath);
}


bool ModuleAssets::persistAsset(Asset* asset, Importer* importer, AssetReference& reference, const std::filesystem::path& sourcePath)
{
    Metadata meta;
    meta.uid = reference.m_uid;
    meta.type = (reference.m_type != AssetType::UNKNOWN) ? reference.m_type: asset->getType();
    meta.sourcePath = sourcePath;

    auto depIt = m_pendingDependencies.find(meta.uid);
    if (depIt != m_pendingDependencies.end())
    {
        meta.m_dependencies = std::move(depIt->second);
        m_pendingDependencies.erase(depIt);
    }

    meta.contentHash = computeMD5(sourcePath);
    reference.m_libId = meta.contentHash;
    asset->setLibId(meta.contentHash);
    reference.m_type = meta.type;

    std::error_code ec;
    meta.sourceFileSize = static_cast<uint64_t>(fs::file_size(sourcePath, ec));
    if (ec) meta.sourceFileSize = 0;

    const auto ftime = fs::last_write_time(sourcePath, ec);
    meta.sourceLastModified = ec ? 0 : static_cast<int64_t>(ftime.time_since_epoch().count());

    std::filesystem::path metaPath = sourcePath;
    Metadata::getMetadataPath(metaPath);
    if (!saveMetaFile(meta, metaPath))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write metadata for '%s'.", sourcePath.string().c_str());
        return false;
    }

    registerIndex(meta.uid, meta.type, sourcePath, meta.contentHash);

    uint8_t* rawBuffer = nullptr;
    const uint64_t size = importer->save(asset, &rawBuffer);
    std::unique_ptr<uint8_t[]> buffer(rawBuffer);

    if (!FileIO::write(meta.getBinaryPath(), buffer.get(), static_cast<size_t>(size)))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write binary for '%s'.", sourcePath.string().c_str());
        return false;
    }

    m_contentRegistry->registerAsset(sourcePath);

    return true;
}

void ModuleAssets::refresh()
{
    std::string rootStr = ASSETS_FOLDER;
    if (!rootStr.empty() && (rootStr.back() == '/' || rootStr.back() == '\\'))
    {
        rootStr.pop_back();
    }

    const fs::path root = rootStr;

    auto tCollect0 = std::chrono::high_resolution_clock::now();
    ScanFileResult scanResult = m_scanner->scan(root);
    auto tCollect1 = std::chrono::high_resolution_clock::now();
    DEBUG_ASSETS("[Module Assets] Scaner took %.3f ms", elapsedMs(tCollect0, tCollect1));


    tCollect0 = std::chrono::high_resolution_clock::now();
    for (const Metadata& meta : scanResult.metadata)
    {
        if (!isValidUID(meta.uid))
        {
            continue;
        }
        registerIndex(meta.uid, meta.type, meta.sourcePath, meta.contentHash);
    }
    tCollect1 = std::chrono::high_resolution_clock::now();
    DEBUG_ASSETS("[Module Assets] Metadata check took %.3f ms", elapsedMs(tCollect0, tCollect1));

    tCollect0 = std::chrono::high_resolution_clock::now();
    // Process assets that need (re)importing.
    for (ImportRequest& req : scanResult.imports)
    {
        AssetReference ref(req.existingUID);
        importAsset(req.sourcePath, ref);
    }
    tCollect1 = std::chrono::high_resolution_clock::now();
    DEBUG_ASSETS("[Module Assets] Metadata reimport loop took %.3f ms", elapsedMs(tCollect0, tCollect1));

    tCollect0 = std::chrono::high_resolution_clock::now();
    // Rebuild the content-browser tree (uses findUID, which is now populated).
    m_contentRegistry->rebuild(root);
    tCollect1 = std::chrono::high_resolution_clock::now();
    DEBUG_ASSETS("[Module Assets] Metadata rebuild took %.3f ms", elapsedMs(tCollect0, tCollect1));
}

void ModuleAssets::registerIndex(const UID& uid, AssetType type,
    const std::filesystem::path& sourcePath,
    const MD5Hash& contentHash)
{
    const fs::path norm = sourcePath.lexically_normal();
    m_uidIndex[uid] = { type, norm, contentHash };
    if (!norm.empty())
    {
        m_pathIndex[norm.string()] = uid;
    }
}

UID ModuleAssets::findUID(const std::filesystem::path& sourcePath) const
{
    const auto it = m_pathIndex.find(sourcePath.lexically_normal().string());
    return it != m_pathIndex.end() ? it->second : INVALID_UID;
}

AssetReference* ModuleAssets::findReference(const UID& uid)
{
    if (!isValidUID(uid))
    {
        return nullptr;
    }

    const auto it = m_uidIndex.find(uid);
    if (it == m_uidIndex.end())
    {
        return nullptr;
    }

    const AssetIndexEntry& entry = it->second;

    if (isValidAsset(entry.contentHash))
    {
        return new AssetReference(uid, entry.contentHash, entry.type);
    }

    if (!entry.sourcePath.empty())
    {
        std::filesystem::path metaPath = entry.sourcePath;
        Metadata::getMetadataPath(metaPath);
        Metadata meta;
        if (loadMetaFile(metaPath, meta))
        {
            const_cast<AssetIndexEntry&>(entry).contentHash = meta.contentHash;
            return new AssetReference(uid, meta.contentHash, meta.type);
        }
    }

    DEBUG_WARN("[ModuleAssets] findReference: UID '%s' found in index but contentHash could not be resolved.", std::to_string(uid).c_str());
    return nullptr;
}

bool ModuleAssets::isLoaded(const AssetReference& ref)
{
    return m_assets.contains(ref.m_uid);
}

void ModuleAssets::unload(const AssetReference& ref)
{
    m_assets.remove(ref.m_uid);
}

void ModuleAssets::registerSubAsset(const Metadata& meta, const UID& parentUID, uint8_t* binaryData, size_t binarySize)
{
    Metadata subMeta = meta;
    subMeta.m_isSubAsset = true;

    if (binaryData && binarySize > 0)
    {
        const std::vector<int8_t> hashInput( reinterpret_cast<const int8_t*>(binaryData), reinterpret_cast<const int8_t*>(binaryData) + binarySize);
        subMeta.contentHash = to_hex_string(computeMD5(hashInput));
    }

    if (!isValidAsset(subMeta.contentHash))
    {
        DEBUG_ERROR("[ModuleAssets] Cannot register sub-asset (UID '%s'): binary data is null or empty.",
            std::to_string(subMeta.uid).c_str());
        return;
    }

    if (binaryData && binarySize > 0)
    {
        if (!FileIO::write(subMeta.getBinaryPath(), binaryData, binarySize))
        {
            DEBUG_ERROR("[ModuleAssets] Failed to write sub-asset binary (UID '%s').",
                std::to_string(subMeta.uid).c_str());
            return;
        }
    }

    m_uidIndex[subMeta.uid] = { subMeta.type, {} };

    if (isValidUID(parentUID))
    {
        DependencyRecord dep;
        dep.uid = subMeta.uid;
        dep.contentHash = subMeta.contentHash;
        dep.type = subMeta.type;
        dep.displayName = subMeta.displayName;
        m_pendingDependencies[parentUID].push_back(dep);
    }
}

bool ModuleAssets::saveMetaFile(const Metadata& meta, const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("uid", meta.uid, alloc);
    doc.AddMember("contentHash", Value(meta.contentHash.c_str(), alloc), alloc);
    doc.AddMember("type", Value(static_cast<uint32_t>(meta.type)), alloc);
    doc.AddMember("sourcePath", Value(meta.sourcePath.string().c_str(), alloc), alloc);
    doc.AddMember("sourceFileSize", Value(meta.sourceFileSize), alloc);
    doc.AddMember("sourceLastModified", Value(meta.sourceLastModified), alloc);

    if (!meta.m_dependencies.empty())
    {
        Value deps(kArrayType);
        for (const DependencyRecord& dep : meta.m_dependencies)
        {
            Value entry(kObjectType);
            entry.AddMember("uid", dep.uid, alloc);
            entry.AddMember("contentHash", Value(dep.contentHash.c_str(), alloc), alloc);
            entry.AddMember("type", Value(static_cast<uint32_t>(dep.type)), alloc);
            if (!dep.displayName.empty())
                entry.AddMember("displayName", Value(dep.displayName.c_str(), alloc), alloc);
            deps.PushBack(entry, alloc);
        }
        doc.AddMember("dependencies", deps, alloc);
    }

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
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
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[AssetMetadata] JSON parse error in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("uid") && doc["uid"].IsUint64())
    {
        outMeta.uid = doc["uid"].GetUint64();
    }
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing 'uid' in '%s'.", pathStr.c_str());
        return false;
    }

    outMeta.contentHash = (doc.HasMember("contentHash") && doc["contentHash"].IsString())
        ? doc["contentHash"].GetString()
        : INVALID_ASSET_ID;

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
        outMeta.sourcePath = doc["sourcePath"].GetString();

    if (doc.HasMember("sourceFileSize") && doc["sourceFileSize"].IsUint64())
        outMeta.sourceFileSize = doc["sourceFileSize"].GetUint64();

    if (doc.HasMember("sourceLastModified") && doc["sourceLastModified"].IsInt64())
        outMeta.sourceLastModified = doc["sourceLastModified"].GetInt64();

    outMeta.m_dependencies.clear();
    if (doc.HasMember("dependencies") && doc["dependencies"].IsArray())
    {
        const auto& deps = doc["dependencies"];
        outMeta.m_dependencies.reserve(deps.Size());
        for (SizeType i = 0; i < deps.Size(); ++i)
        {
            const auto& entry = deps[i];
            if (!entry.HasMember("uid") || !entry["uid"].IsUint64())  continue;
            if (!entry.HasMember("type") || !entry["type"].IsNumber()) continue;

            DependencyRecord rec;
            rec.uid = entry["uid"].GetUint64();
            rec.type = static_cast<AssetType>(entry["type"].GetUint());
            if (entry.HasMember("contentHash") && entry["contentHash"].IsString())
                rec.contentHash = entry["contentHash"].GetString();
            if (entry.HasMember("displayName") && entry["displayName"].IsString())
                rec.displayName = entry["displayName"].GetString();

            outMeta.m_dependencies.push_back(std::move(rec));
        }
    }

    return true;
}

void ModuleAssets::requestSave(Asset& asset)
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

    std::optional<fs::path> result;
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

bool ModuleAssets::createStateMachineFromGltf(const std::filesystem::path& gltfPath)
{
    return m_importerGltf->createStateMachine(gltfPath);
}

ContentRegistry* ModuleAssets::getContentRegistry() const
{
    return m_contentRegistry.get();
}

PrefabManager* ModuleAssets::getPrefabManager() const
{
    return m_prefabManager.get();
}
