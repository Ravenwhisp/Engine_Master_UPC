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
#include "GenericTypeFactory.h"
#include "AssetReferenceRepair.h"

#include <filesystem>
#include <FileIO.h>
#include <algorithm>
#include <map>
#include <set>
#include <exception>

namespace fs = std::filesystem;

namespace
{
    bool writeLibraryData(const fs::path& path, const uint8_t* data, size_t size)
    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        std::string error = ec.message();
        if (ec || !AssetReferenceRepair::writeAtomically(path, reinterpret_cast<const char*>(data), size, error))
        {
            DEBUG_ERROR("[ModuleAssets] Library write failed for '%s': %s.", path.string().c_str(), error.c_str());
            return false;
        }
        return true;
    }
}

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

bool ModuleAssets::importAsset(const std::filesystem::path& sourcePath, AssetId& reference)
{
    return importAssetInternal(sourcePath, reference);
}

bool ModuleAssets::importAssetInternal(const std::filesystem::path& sourcePath, AssetId& reference,
                                     std::unique_ptr<Asset>* retainedAsset)
try
{
    // Callers can pass a path from the index; importing may re-hash that map.
    const std::filesystem::path importPath = sourcePath;
    Importer* importer = m_importers.findByPath(importPath);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] No importer found for '%s'.", sourcePath.string().c_str());
        reference = AssetId();
        return false;
    }

    const bool isReimport = isValidUID(reference.m_uid);
    UID uid = isReimport ? reference.m_uid : GenerateUID();

    reference.m_uid = uid;
    reference.m_type = importer->getAssetType();
    reference.m_libId = INVALID_ASSET_ID;
    m_pendingDependencies.erase(uid);
    m_failedSubAssetImports.erase(uid);

    std::unique_ptr<Asset> asset;

    if (reference.m_type == AssetType::DATA_CONTAINER)
    {
        JsonArchive archive(ArchiveMode::Input);
        if (archive.loadFile(importPath))
        {
            std::string typeName;
            if (archive.read("_typeName", typeName) && DataContainerFactory::isRegistered(typeName))
            {
                asset = DataContainerFactory::create(typeName, reference);
            }
        }
    }

    if (!asset)
    {
        asset.reset(importer->createAssetInstance(reference));
    }
    if (!asset)
    {
        DEBUG_ERROR("[ModuleAssets] Could not create asset for '%s'.", importPath.string().c_str());
        return false;
    }

    if (auto cached = m_cache.get(reference.m_uid))
    {
        if (cached->getImportSettings())
        {
            asset->setImportSettings(cached->getImportSettings()->clone());
        }
    }
    if (!asset->getImportSettings())
    {
        std::filesystem::path metaPath = importPath;
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

    if (!importer->import(importPath, asset.get()) || m_failedSubAssetImports.count(uid) != 0)
    {
        DEBUG_ERROR("[ModuleAssets] Import failed for '%s'.", importPath.string().c_str());
        if (!isReimport) reference = AssetId();
        m_pendingDependencies.erase(uid);
        m_failedSubAssetImports.erase(uid);
        return false;
    }

    if (!persistAsset(asset.get(), importer, reference, importPath))
    {
        if (!isReimport) reference = AssetId();
        m_pendingDependencies.erase(uid);
        return false;
    }
    if (retainedAsset) *retainedAsset = std::move(asset);
    return true;
}
catch (const std::exception& exception)
{
    DEBUG_ERROR("[ModuleAssets] Import failed for UID %llu: %s.",
        static_cast<unsigned long long>(reference.m_uid), exception.what());
    m_pendingDependencies.erase(reference.m_uid);
    m_failedSubAssetImports.erase(reference.m_uid);
    reference.m_libId = INVALID_ASSET_ID;
    return false;
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
    if (!isValidAsset(contentHash))
    {
        DEBUG_ERROR("[ModuleAssets] Cannot hash '%s'.", sourcePath.string().c_str());
        return false;
    }

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
        meta.m_dependencies = deps->second;
    }

    // Write the binary before publishing its metadata/hash. Keep old versions:
    // they may still be referenced by unsaved scenes or share another UID.
    {
        uint8_t* binaryBuffer = nullptr;
        const uint64_t binarySize = importer->save(asset, &binaryBuffer);
        std::unique_ptr<uint8_t[]> bufferGuard(binaryBuffer);
        if (!binaryBuffer || binarySize == 0 ||
            !writeLibraryData(meta.getBinaryPath(), binaryBuffer, static_cast<size_t>(binarySize)))
        {
            DEBUG_ERROR("[ModuleAssets] Failed to serialize/write library data for '%s'.", sourcePath.string().c_str());
            return false;
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
    m_pendingDependencies.erase(reference.m_uid);
    reference.m_libId = meta.contentHash;
    asset->setUID(reference.m_uid);
    asset->setLibId(reference.m_libId);
    m_cache.unload(reference.m_uid);

#ifndef GAME_RELEASE
    m_contentRegistry->registerAsset(sourcePath, &m_index);
#endif

    return true;
}

void ModuleAssets::refresh()
{
    refreshIndex(true);
}

void ModuleAssets::refreshIndex(bool importChangedSources)
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
        if (importChangedSources)
        {
            AssetId ref(req.existingUID);
            importAsset(req.sourcePath, ref);
        }
        else if (!isValidUID(m_index.findUID(req.sourcePath)))
        {
            // Include newly discovered sources in the ordered fix pass without
            // letting the scanner import prefabs ahead of their dependencies.
            Importer* importer = m_importers.findByPath(req.sourcePath);
            if (importer)
                m_index.registerEntry(isValidUID(req.existingUID) ? req.existingUID : GenerateUID(),
                    importer->getAssetType(), req.sourcePath);
        }
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

    if (!binaryData || binarySize == 0)
    {
        DEBUG_ERROR("[ModuleAssets] Cannot register sub-asset (UID '%s'): binary data is null or empty.",
            std::to_string(subMeta.uid).c_str());
        if (isValidUID(parentUID)) m_failedSubAssetImports.insert(parentUID);
        return;
    }

    const std::vector<int8_t> hashInput(reinterpret_cast<const int8_t*>(binaryData),
        reinterpret_cast<const int8_t*>(binaryData) + binarySize);
    subMeta.contentHash = to_hex_string(computeMD5(hashInput));
    if (!writeLibraryData(subMeta.getBinaryPath(), binaryData, binarySize))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write sub-asset binary (UID '%s').",
            std::to_string(subMeta.uid).c_str());
        if (isValidUID(parentUID)) m_failedSubAssetImports.insert(parentUID);
        return;
    }

    m_index.registerEntry(subMeta.uid, subMeta.type, {}, subMeta.contentHash);
    m_cache.unload(subMeta.uid);

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

AssetReferenceFixResult ModuleAssets::fixAllAssetReferences()
{
    AssetReferenceFixResult result;
    DEBUG_LOG("[ModuleAssets] Fix pass: indexing sources without unordered reimports...");
    refreshIndex(false);

    struct Source
    {
        UID uid;
        std::filesystem::path path;
        Importer* importer;
    };
    std::vector<Source> external;
    std::map<UID, std::filesystem::path> sourcePaths;
    std::set<UID> failed;
    auto fail = [&](UID uid, const std::filesystem::path& path, const char* reason)
    {
        if (failed.insert(uid).second) ++result.failedAssets;
        DEBUG_ERROR("[ModuleAssets] Fix pass: '%s': %s.", path.string().c_str(), reason);
    };
    for (const auto& [uid, entry] : m_index.allEntries())
    {
        if (entry.sourcePath.empty()) continue;
        sourcePaths[uid] = entry.sourcePath;
        Importer* importer = m_importers.findByPath(entry.sourcePath);
        if (!importer) fail(uid, entry.sourcePath, "No source importer");
        else if (!importer->isNative()) external.push_back({uid, entry.sourcePath, importer});
    }

    // glTFs produce PREFABs too: sort by source importer, not output type.
    Importer* gltfImporter = m_importers.getGltfImporter();
    std::sort(external.begin(), external.end(), [&](const Source& a, const Source& b)
    {
        if ((a.importer == gltfImporter) != (b.importer == gltfImporter))
            return b.importer == gltfImporter;
        return a.path < b.path;
    });
    std::map<UID, std::unique_ptr<Asset>> retainedGltfs;
    for (const Source& source : external)
    {
        AssetId ref(source.uid);
        std::unique_ptr<Asset> retained;
        if (!importAssetInternal(source.path, ref, source.importer == gltfImporter ? &retained : nullptr))
            fail(source.uid, source.path, "External import failed");
        else
        {
            ++result.successfulImports;
            if (retained) retainedGltfs.emplace(source.uid, std::move(retained));
        }
    }

    // Re-snapshot after external importers create native assets (state machines).
    std::map<UID, std::filesystem::path> native;
    for (const auto& [uid, entry] : m_index.allEntries())
    {
        if (entry.sourcePath.empty()) continue;
        sourcePaths[uid] = entry.sourcePath;
        Importer* importer = m_importers.findByPath(entry.sourcePath);
        if (importer && importer->isNative()) native.emplace(uid, entry.sourcePath);
    }

    std::map<UID, std::unique_ptr<rapidjson::Document>> documents;
    std::map<UID, std::set<UID>> graph;
    for (const auto& [uid, path] : native)
    {
        graph[uid];
        JsonArchive archive(ArchiveMode::Input);
        if (!archive.loadFile(path) || !archive.currentInput()->IsObject())
        {
            fail(uid, path, "Invalid native JSON");
            continue;
        }
        auto document = std::make_unique<rapidjson::Document>();
        document->CopyFrom(*archive.currentInput(), document->GetAllocator());
        AssetReferenceRepair::visitReferences(*document, [&](const rapidjson::Value&, UID dependency, const std::string&)
        {
            graph[uid].insert(dependency);
        });
        documents.emplace(uid, std::move(document));
    }

    // A failed glTF must also block consumers of its mesh/material sub-assets.
    std::set<UID> unavailable = failed;
    for (UID uid : failed)
    {
        auto path = sourcePaths.find(uid);
        if (path == sourcePaths.end()) continue;
        auto metaPath = path->second;
        Metadata::getMetadataPath(metaPath);
        JsonArchive archive(ArchiveMode::Input);
        Metadata meta;
        if (archive.loadFile(metaPath))
        {
            meta.serialize(archive);
            for (const auto& dependency : meta.m_dependencies) unavailable.insert(dependency.uid);
        }
    }

    const auto order = AssetReferenceRepair::orderDependencies(graph);
    for (UID uid : order.blocked)
    {
        ++result.cycleSkippedAssets;
        unavailable.insert(uid);
        DEBUG_WARN("[ModuleAssets] Fix pass: '%s' skipped: cyclic dependency or depends on a cycle.",
            native.at(uid).string().c_str());
    }
    for (UID uid : order.ordered)
    {
        const auto& path = native.at(uid);
        if (failed.count(uid)) continue;
        if (std::any_of(graph.at(uid).begin(), graph.at(uid).end(),
            [&](UID dependency) { return unavailable.count(dependency) != 0; }))
        {
            fail(uid, path, "Dependency import/repair failed");
            unavailable.insert(uid);
            continue;
        }
        auto& document = *documents.at(uid);
        const auto counts = AssetReferenceRepair::repairReferences(document, [&](UID dependency) -> const std::string*
        {
            const AssetIndexEntry* entry = m_index.findEntry(dependency);
            return entry ? &entry->contentHash : nullptr;
        }, [&](UID dependency, const std::string& location)
        {
            DEBUG_WARN("[ModuleAssets] Fix pass: '%s' at %s: UID %llu has no indexed hash; preserved.",
                path.string().c_str(), location.c_str(), static_cast<unsigned long long>(dependency));
        });
        result.unresolvedReferences += counts.unresolved;
        if (counts.repaired != 0)
        {
            std::string error;
            if (!AssetReferenceRepair::saveAtomically(document, path, error))
            {
                fail(uid, path, error.c_str());
                unavailable.insert(uid);
                continue;
            }
            result.repairedReferences += counts.repaired;
            ++result.changedSourceFiles;
        }
        AssetId ref(uid);
        if (!importAsset(path, ref))
        {
            fail(uid, path, "Native import failed");
            unavailable.insert(uid);
        }
        else ++result.successfulImports;
    }

    // Only reserialize retained prefab data: reimporting the glTF here would
    // regenerate sub-assets after their consumers have already been repaired.
    for (const auto& [uid, asset] : retainedGltfs)
    {
        uint8_t* binaryBuffer = nullptr;
        const uint64_t binarySize = gltfImporter->save(asset.get(), &binaryBuffer);
        std::unique_ptr<uint8_t[]> bufferGuard(binaryBuffer);
        const auto binaryPath = (fs::path(LIBRARY_FOLDER) / asset->getLibId()) += ASSET_EXTENSION;
        if (!binaryBuffer || binarySize == 0 ||
            !writeLibraryData(binaryPath, binaryBuffer, static_cast<size_t>(binarySize)))
        {
            fail(uid, sourcePaths.at(uid), "Final glTF library serialization failed");
            --result.successfulImports;
        }
        else m_cache.unload(uid);
    }
    DEBUG_LOG("[ModuleAssets] Fix pass: %zu reference(s), %zu source(s), %zu import(s), "
              "%zu unresolved, %zu failed, %zu cycle skips.", result.repairedReferences,
              result.changedSourceFiles, result.successfulImports, result.unresolvedReferences,
              result.failedAssets, result.cycleSkippedAssets);
    return result;
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
