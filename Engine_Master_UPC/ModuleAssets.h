#pragma once
#include "Module.h"
#include "UID.h"
#include "AssetsDictionary.h"
#include "WeakCache.h"

#include "PrefabAsset.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <mutex>
#include "Metadata.h"
#include "AssetReference.h"
#include "FileIO.h"
#include "Importer.h"


class AssetScanner;
class ContentRegistry;
class PrefabManager;

class Asset;

class ImporterTexture;
class ImporterMaterial;
class ImporterMesh;
class ImporterPrefab;
class ImporterAnimation;
class ImporterSkin;
class ImporterAnimationStateMachine;
class ImporterGltf;
class ImporterFont;
class ImporterScene;

struct ScanFileResult;


struct AssetIndexEntry
{
    AssetType type = AssetType::UNKNOWN;
    std::filesystem::path sourcePath;
    MD5Hash contentHash = INVALID_ASSET_ID;
};


class ModuleAssets : public Module
{
private:
    std::unordered_map<UID, AssetIndexEntry>  m_uidIndex;
    std::unordered_map<std::string, UID>      m_pathIndex;

    std::unique_ptr<AssetScanner>       m_scanner;
    std::unique_ptr<ContentRegistry>    m_contentRegistry;
    std::unique_ptr<PrefabManager>      m_prefabManager;

    // Runtime cache keyed by stable UID.
    WeakCache<UID, Asset>               m_assets;

public:
    ModuleAssets();
    ~ModuleAssets();

#pragma region GameLoop
    bool init() override;
    void postRender() override;
    bool cleanUp() override;
#pragma endregion

#pragma region Importer
    Importer* findImporter(const std::filesystem::path& filePath) const;
    Importer* findImporter(AssetType type) const;
    bool canImport(const std::filesystem::path& sourcePath) const;
#pragma endregion

    void importAsset(const std::filesystem::path& sourcePath, AssetReference& reference);
    bool save(Asset& asset, const std::filesystem::path& path = {});
    void refresh();

    ContentRegistry* getContentRegistry() const;
    PrefabManager* getPrefabManager() const;

    // Loads an asset by its stable UID.
    template<typename T>
    std::shared_ptr<T> load(AssetReference& ref)
    {
        if (!isValidUID(ref.m_uid))
        {
            return nullptr;
        }

        if (auto cached = m_assets.getAs<T>(ref.m_uid))
        {
            return cached;
        }

        const auto idxIt = m_uidIndex.find(ref.m_uid);

        if (isValidAsset(ref.m_libId))
        {
            if (ref.m_type == AssetType::UNKNOWN && idxIt != m_uidIndex.end())
            {
                ref.m_type = idxIt->second.type;
            }

            if (idxIt != m_uidIndex.end() && isValidAsset(idxIt->second.contentHash))
            {
                ref.m_libId = idxIt->second.contentHash;
            }

            if (auto loaded = loadFromLibrary<T>(ref))
            {
                return loaded;
            }
        }

        if (idxIt == m_uidIndex.end() || idxIt->second.sourcePath.empty())
        {
            DEBUG_ERROR("[ModuleAssets] Cannot load UID '%s': no source path available for re-import.", std::to_string(ref.m_uid).c_str());
            return nullptr;
        }

        importAsset(idxIt->second.sourcePath, ref);
        if (!isValidAsset(ref.m_libId))
        {
            return nullptr;
        }

        return loadFromLibrary<T>(ref);
    }

    template<typename T>
    std::shared_ptr<T> loadAtPath(const std::filesystem::path& sourcePath)
    {
        const UID uid = findUID(sourcePath);
        if (isValidUID(uid))
        {
            if (auto cached = m_assets.getAs<T>(uid))
            {
                return cached;
            }
        }

        std::filesystem::path metaPath = sourcePath;
        Metadata::getMetadataPath(metaPath);

        Metadata meta;
        if (loadMetaFile(metaPath, meta))
        {
            registerIndex(meta.uid, meta.type, sourcePath);
            AssetReference ref(meta.uid, meta.contentHash, meta.type);
            return load<T>(ref);
        }

        AssetReference ref(isValidUID(uid) ? uid : INVALID_UID);
        importAsset(sourcePath, ref);
        if (!ref.isValid())
        {
            return nullptr;
        }

        return loadFromLibrary<T>(ref);
    }

    bool isLoaded(const AssetReference& id);
    void unload(const AssetReference& id);

    std::optional<AssetReference> findReference(const UID& uid);

    UID findUID(const std::filesystem::path& sourcePath) const;

    bool saveMetaFile(const Metadata& meta, const std::filesystem::path& metaPath);
    bool loadMetaFile(const std::filesystem::path& metaPath, Metadata& outMeta);

    void registerSubAsset(const Metadata& meta, const UID& parentUID,  uint8_t* binaryData, size_t binarySize);

    void flushDialogRequests();

    bool createStateMachineFromGltf(const std::filesystem::path& gltfPath);

private:
    
    template<typename T>
    std::shared_ptr<T> loadFromLibrary(AssetReference& ref)
    {
        if (ref.m_type == AssetType::UNKNOWN)
        {
            return nullptr;
        }

        Importer* importer = findImporter(ref.m_type);
        if (!importer)
        {
            DEBUG_ERROR("[ModuleAssets] No importer for type %u (UID '%s').", static_cast<uint32_t>(ref.m_type), std::to_string(ref.m_uid).c_str());
            return nullptr;
        }

        const std::filesystem::path binaryPath = buildLibraryPath(ref.m_libId);

        const std::vector<uint8_t> buffer = FileIO::read(binaryPath);
        if (buffer.empty())
        {
            return nullptr;
        }

        std::shared_ptr<Asset> asset(importer->createAssetInstance(ref));
        importer->load(buffer.data(), asset.get());
        m_assets.insert(ref.m_uid, asset);

        return std::static_pointer_cast<T>(asset);
    }

    void requestSave(Asset& asset);
    bool persistAsset(Asset* asset, Importer* importer, AssetReference& reference, const std::filesystem::path& sourcePath);
   
    void registerIndex(const UID& uid, AssetType type, const std::filesystem::path& sourcePath, const MD5Hash& contentHash = INVALID_ASSET_ID);



#pragma region Importers
    ImporterTexture* m_importerTexture = nullptr;
    ImporterMesh* m_importerMesh = nullptr;
    ImporterMaterial* m_importerMaterial = nullptr;
    ImporterPrefab* m_importerPrefab = nullptr;
    ImporterAnimation* m_importerAnimation = nullptr;
    ImporterSkin* m_importerSkin = nullptr;
    ImporterGltf* m_importerGltf = nullptr;
    ImporterFont* m_importerFont = nullptr;
    ImporterScene* m_importerScene = nullptr;
    ImporterAnimationStateMachine* m_importerAnimationStateMachine = nullptr;

    std::vector<Importer*> m_importers;
#pragma endregion

    std::unordered_map<UID, std::vector<DependencyRecord>> m_pendingDependencies;

#pragma region FileDialog
    std::atomic<bool>                                           m_dialogRunning{ false };
    std::mutex                                                  m_dialogResultMutex;
    std::optional<std::filesystem::path>                        m_dialogResult;
    std::function<void(const std::filesystem::path&)>           m_dialogCallback;
    Asset*                                                      m_pendingAsset = nullptr;
    AssetType                                                   m_pendingAssetType = AssetType::UNKNOWN;
    std::atomic<bool>                                           m_pendingIsSave{ false };
#pragma endregion

};