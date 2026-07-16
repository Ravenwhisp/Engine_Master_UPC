#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "Importer.h"
#include "ImporterGltf.h"
#include "MD5.h"

#ifndef GAME_RELEASE
#include "AssetScanner.h"
#include "ContentRegistry.h"
#endif
#include "PrefabManager.h"


#include "Prefab.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "ModuleScene.h"

#include "Asset.h"
#include "AnimationStateMachineAsset.h"
#include "JsonArchive.h"
#include "Metadata.h"
#include "UID.h"
#include "DataContainer.h"

#include <filesystem>
#include <FileIO.h>

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
#ifndef GAME_RELEASE
    m_scanner = std::make_unique<AssetScanner>();
    m_contentRegistry = std::make_unique<ContentRegistry>();
#endif
    m_prefabManager = std::make_unique<PrefabManager>(this);

#ifndef GAME_RELEASE
    refresh();
#endif
    return true;
}

void ModuleAssets::postRender()
{
#ifndef GAME_RELEASE
    m_dialog.flush(*this);
#endif
}

bool ModuleAssets::cleanUp()
{
    m_cache.clear();
    return true;
}

bool ModuleAssets::canImport(const std::filesystem::path& sourcePath) const
{
    return m_importers.canImport(sourcePath);
}

void ModuleAssets::importAsset(const std::filesystem::path& sourcePath, AssetId& reference)
{
    Importer* importer = m_importers.findByPath(sourcePath);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] No importer found for '%s'.", sourcePath.string().c_str());
        reference = AssetId();
        return;
    }

    const bool isReimport = isValidUID(reference.m_uid);
    UID uid = isReimport ? reference.m_uid : GenerateUID();

    reference.m_uid = uid;
    reference.m_type = importer->getAssetType();
    reference.m_libId = INVALID_ASSET_ID;

    std::unique_ptr<Asset> asset(importer->createAssetInstance(reference));

    if (auto cached = m_cache.get(reference.m_uid))
    {
        if (cached->getImportSettings())
        {
            asset->setImportSettings(cached->getImportSettings()->clone());
        }
    }
    if (!asset->getImportSettings())
    {
        std::filesystem::path metaPath = sourcePath;
        Metadata::getMetadataPath(metaPath);
        if (fs::exists(metaPath))
        {
            Metadata existingMeta;
            JsonArchive archive(ArchiveMode::Input);
            if (archive.loadFile(metaPath))
            {
                existingMeta.serialize(archive);
                if (existingMeta.importSettings)
                {
                    asset->setImportSettings(std::move(existingMeta.importSettings));
                }
            }
        }
    }
    if (!asset->getImportSettings())
    {
        asset->setImportSettings(asset->createDefaultImportSettings());
    }

    if (!importer->import(sourcePath, asset.get()))
    {
        DEBUG_ERROR("[ModuleAssets] Import failed for '%s'.", sourcePath.string().c_str());
        if (!isReimport) reference = AssetId();
        return;
    }

    if (reference.m_type == AssetType::DATA_CONTAINER)
    {
        DataContainer* baseDc = static_cast<DataContainer*>(asset.release());
        DataContainer* derivedDc = resolveDataContainerType(baseDc);
        if (derivedDc)
        {
            delete baseDc;
            asset.reset(derivedDc);
        }
        else
        {
            asset.reset(baseDc);
        }
    }

    if (!persistAsset(asset.get(), importer, reference, sourcePath))
    {
        if (!isReimport) reference = AssetId();
    }
}

bool ModuleAssets::save(Asset& asset, const std::filesystem::path& path)
{
    std::filesystem::path targetPath = path;

    if (targetPath.empty())
    {
        const AssetIndexEntry* entry = m_index.findEntry(asset.getUID());
        if (entry && !entry->sourcePath.empty())
        {
            targetPath = entry->sourcePath;
        }
    }

    if (targetPath.empty())
    {
#ifndef GAME_RELEASE
        m_dialog.requestSave(asset);
#endif
        return false;
    }

    Importer* importer = m_importers.findByType(asset.getType());
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
    AssetId ref(uid, INVALID_ASSET_ID, asset.getType());
    if (persistAsset(&asset, importer, ref, targetPath))
    {
        asset.setUID(ref.m_uid);
        asset.setLibId(ref.m_libId);
        return true;
    }
    return false;
}

bool ModuleAssets::persistAsset(Asset* asset, Importer* importer, AssetId& reference,
                                 const std::filesystem::path& sourcePath)
{
    const MD5Hash sourceHash = computeMD5(sourcePath);
    const MD5Hash contentHash = isValidAsset(sourceHash) ? sourceHash : reference.m_libId;

    Metadata meta;
    meta.uid = reference.m_uid;
    meta.type = reference.m_type;
    meta.sourcePath = sourcePath;
    meta.contentHash = contentHash;
    meta.importSettings = asset->getImportSettings() ? asset->getImportSettings()->clone() : nullptr;

    if (fs::exists(sourcePath))
    {
        meta.sourceFileSize = fs::file_size(sourcePath);
    }

    auto deps = m_pendingDependencies.find(reference.m_uid);
    if (deps != m_pendingDependencies.end())
    {
        meta.m_dependencies = std::move(deps->second);
        m_pendingDependencies.erase(deps);
    }

    {
        AssetIndexEntry* prevEntry = m_index.findEntryMutable(meta.uid);
        if (prevEntry && isValidAsset(prevEntry->contentHash) && prevEntry->contentHash != meta.contentHash)
        {
            FileIO::remove(std::filesystem::path(LIBRARY_FOLDER) / prevEntry->contentHash += ASSET_EXTENSION);
        }
    }

    std::filesystem::path metaPath = sourcePath;
    Metadata::getMetadataPath(metaPath);
    {
        JsonArchive archive;
        meta.serialize(archive);
        if (!archive.saveFile(metaPath))
        {
            return false;
        }
    }

    m_index.registerEntry(meta.uid, meta.type, sourcePath, meta.contentHash);

    uint8_t* binaryBuffer = nullptr;
    const uint64_t binarySize = importer->save(asset, &binaryBuffer);
    if (binaryBuffer && binarySize > 0)
    {
        const std::filesystem::path binaryPath = meta.getBinaryPath();
        FileIO::write(binaryPath, binaryBuffer, binarySize);
        delete[] binaryBuffer;
    }

    reference.m_libId = meta.contentHash;

#ifndef GAME_RELEASE
    m_contentRegistry->registerAsset(sourcePath, &m_index);
#endif

    return true;
}

void ModuleAssets::refresh()
{
#ifndef GAME_RELEASE
    std::string rootStr = ASSETS_FOLDER;
    if (!rootStr.empty() && (rootStr.back() == '/' || rootStr.back() == '\\'))
    {
        rootStr.pop_back();
    }

    const fs::path root = rootStr;

    auto tCollect0 = std::chrono::high_resolution_clock::now();
    ScanFileResult scanResult = m_scanner->scan(root);
    auto tCollect1 = std::chrono::high_resolution_clock::now();
    DEBUG_ASSETS("[ModuleAssets] Scaner took %.3f ms", elapsedMs(tCollect0, tCollect1));

    tCollect0 = std::chrono::high_resolution_clock::now();
    for (const Metadata& meta : scanResult.metadata)
    {
        if (!isValidUID(meta.uid))
        {
            continue;
        }
        m_index.registerEntry(meta.uid, meta.type, meta.sourcePath, meta.contentHash);

        for (const auto& dep : meta.m_dependencies)
        {
            if (isValidUID(dep.uid))
                m_index.registerEntry(dep.uid, dep.type, {}, dep.contentHash);
        }
    }
    tCollect1 = std::chrono::high_resolution_clock::now();
    DEBUG_ASSETS("[ModuleAssets] Metadata check took %.3f ms", elapsedMs(tCollect0, tCollect1));

    tCollect0 = std::chrono::high_resolution_clock::now();
    for (ImportRequest& req : scanResult.imports)
    {
        AssetId ref(req.existingUID);
        importAsset(req.sourcePath, ref);
    }
    tCollect1 = std::chrono::high_resolution_clock::now();
    DEBUG_ASSETS("[ModuleAssets] Metadata reimport loop took %.3f ms", elapsedMs(tCollect0, tCollect1));

    tCollect0 = std::chrono::high_resolution_clock::now();
    m_contentRegistry->rebuild(root, &m_index);
    tCollect1 = std::chrono::high_resolution_clock::now();
    DEBUG_ASSETS("[ModuleAssets] Metadata rebuild took %.3f ms", elapsedMs(tCollect0, tCollect1));
#endif
}

void ModuleAssets::unregisterAsset(const fs::path& sourcePath)
{
    const fs::path normPath = sourcePath.lexically_normal();
    m_index.unregisterByPath(normPath);
#ifndef GAME_RELEASE
    m_contentRegistry->unregisterAsset(normPath);
#endif
}

bool ModuleAssets::isLoaded(const AssetId& ref)
{
    return m_cache.isLoaded(ref.m_uid);
}

void ModuleAssets::unload(const AssetId& ref)
{
    m_cache.unload(ref.m_uid);
}

void ModuleAssets::registerSubAsset(const Metadata& meta, const UID& parentUID,
                                     uint8_t* binaryData, size_t binarySize)
{
    Metadata subMeta = meta;
    subMeta.m_isSubAsset = true;

    if (binaryData && binarySize > 0)
    {
        const std::vector<int8_t> hashInput(reinterpret_cast<const int8_t*>(binaryData),
            reinterpret_cast<const int8_t*>(binaryData) + binarySize);
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

    {
        AssetIndexEntry* prevEntry = m_index.findEntryMutable(subMeta.uid);
        if (prevEntry)
        {
            const MD5Hash& prevHash = prevEntry->contentHash;
            if (isValidAsset(prevHash) && prevHash != subMeta.contentHash)
            {
                FileIO::remove(std::filesystem::path(LIBRARY_FOLDER) / prevHash += ASSET_EXTENSION);
            }
        }
    }
    m_index.registerEntry(subMeta.uid, subMeta.type, {}, subMeta.contentHash);

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

AssetId* ModuleAssets::findReference(const UID& uid)
{
    if (!isValidUID(uid))
    {
        return nullptr;
    }

    const AssetIndexEntry* entry = m_index.findEntry(uid);
    if (!entry)
    {
        return nullptr;
    }

    if (isValidAsset(entry->contentHash))
    {
        return new AssetId(uid, entry->contentHash, entry->type);
    }

    if (!entry->sourcePath.empty())
    {
        std::filesystem::path metaPath = entry->sourcePath;
        Metadata::getMetadataPath(metaPath);
        Metadata meta;
        JsonArchive archive(ArchiveMode::Input);
        if (archive.loadFile(metaPath))
        {
            meta.serialize(archive);
            AssetIndexEntry* mutableEntry = m_index.findEntryMutable(uid);
            if (mutableEntry)
            {
                mutableEntry->contentHash = meta.contentHash;
            }
            return new AssetId(uid, meta.contentHash, meta.type);
        }
    }

    DEBUG_WARN("[ModuleAssets] findReference: UID '%s' found in index but contentHash could not be resolved.",
        std::to_string(uid).c_str());
    return nullptr;
}

bool ModuleAssets::createStateMachineFromGltf(const std::filesystem::path& gltfPath)
{
    ImporterGltf* gltfImporter = m_importers.getGltfImporter();
    if (!gltfImporter) return false;
    return gltfImporter->createStateMachine(gltfPath);
}

ContentRegistry* ModuleAssets::getContentRegistry() const
{
#ifndef GAME_RELEASE
    return m_contentRegistry.get();
#else
    return nullptr;
#endif
}

PrefabManager* ModuleAssets::getPrefabManager() const
{
    return m_prefabManager.get();
}

DataContainer* ModuleAssets::resolveDataContainerType(DataContainer* baseContainer) const
{
    if (!baseContainer)
    {
        return nullptr;
    }

    const rapidjson::Document& data = baseContainer->getData();
    if (!data.HasMember("_typeName") || !data["_typeName"].IsString())
    {
        return nullptr;
    }

    const char* typeName = data["_typeName"].GetString();
    AssetId ref = baseContainer->getReference();

    auto derived = DataContainerFactory::create(typeName, ref);
    if (!derived)
    {
        return nullptr;
    }

    JsonArchive archive(ArchiveMode::Input);
    archive.setValue(data);
    derived->serialize(archive);
    return derived.release();
}
