#pragma once
#include "Module.h"
#include "UID.h"
#include "AssetsDictionary.h"
#include "WeakCache.h"
#include "AssetRegistry.h"

#include "AssetScanner.h"
#include "ContentRegistry.h"
#include "FileDialogRequest.h"

#include <filesystem>
#include <memory>
#include <Metadata.h>
#include <mutex>

class Asset;
class Importer;
class ImporterTexture;
class ImporterGltf;
class ImporterMaterial;
class ImporterMesh;
class ImporterPrefab;
class ImporterAnimation;
class ImporterSkin;
class ImporterAnimationStateMachine;
class ImporterFont;
class AnimationStateMachineAsset;
class ImporterFont;
struct FileEntry;
class ISerializable;


class ModuleAssets : public Module
{
public:
    ~ModuleAssets();

#pragma region GameLoop
    bool init() override;
    bool cleanUp() override;
#pragma endregion

    bool canImport(const std::filesystem::path& sourcePath) const;
    void importAsset(const std::filesystem::path& sourcePath, MD5Hash& uid);
    void refresh();

    bool save(const Asset& asset, const std::filesystem::path& path = {});
    bool loadMetadata(const std::filesystem::path& path, Metadata& out);

    template<typename T>
    std::shared_ptr<T> load(MD5Hash id)
    {
        if (auto cached = m_assets.getAs<T>(id))
        {
            return cached;
        }

        const Metadata* meta = m_registry->getMetadata(id);
        if (!meta)
        {
            DEBUG_ERROR("[ModuleAssets] No metadata found for UID %llu.", id);
            return nullptr;
        }

        return std::static_pointer_cast<T>(loadAsset(meta));
    }

    template<typename T>
    std::shared_ptr<T> loadAtPath(const std::filesystem::path& sourcePath)
    {
        const MD5Hash id = m_registry->findByPath(sourcePath);
        if (id == INVALID_ASSET_ID)
        {
            DEBUG_ERROR("[ModuleAssets] No asset registered at path '%s'.", sourcePath.string().c_str());
            return nullptr;
        }
        return load<T>(id);
    }

    MD5Hash                     findUID(const std::filesystem::path& sourcePath) const;
    bool                        isLoaded(const MD5Hash& id);
    void                        unload(const MD5Hash& id);

    std::shared_ptr<FileEntry>  getRoot() const;
    std::shared_ptr<FileEntry>  getEntry(const std::filesystem::path&) const;

    void                        registerSubAsset(const Metadata& meta, const MD5Hash& parentUID, uint8_t* binaryData, size_t binarySize);

#pragma region Importer
    Importer*                   findImporter(const std::filesystem::path& filePath) const;
    Importer*                   findImporter(AssetType type) const;
#pragma endregion

    void flushDialogRequests();
private:

    bool persistAsset(const Asset* asset, Importer* importer, const MD5Hash& uid, const std::filesystem::path& sourcePath);
    bool isDialogOpen() const { return m_dialogRunning.load(); }

    std::shared_ptr<Asset>  loadAsset(const Metadata* metadata);
    bool                    writeMetadata(const Metadata& meta, const std::filesystem::path& metaPath);
    void                    requestSave(const Asset& asset);

    std::unique_ptr<AssetRegistry>                              m_registry;
    std::unique_ptr<AssetScanner>                               m_scanner;
    std::unique_ptr<ContentRegistry>                            m_contentRegistry;
    WeakCache<MD5Hash, Asset>                                   m_assets;
    std::unordered_map<MD5Hash, std::vector<DependencyRecord>>  m_pendingDependencies;

#pragma region Importers
    ImporterTexture*                                            m_importerTexture = nullptr;
    ImporterMesh*                                               m_importerMesh = nullptr;
    ImporterMaterial*                                           m_importerMaterial = nullptr;
    ImporterPrefab*                                             m_importerPrefab = nullptr;
    ImporterAnimation*                                          m_importerAnimation = nullptr;
    ImporterSkin*                                               m_importerSkin = nullptr;
    ImporterGltf*                                               m_importerGltf = nullptr;
	ImporterFont*                                               m_importerFont = nullptr;
    ImporterAnimationStateMachine*                              m_importerAnimationStateMachine = nullptr;

    std::vector<Importer*>                                      m_importers;
#pragma endregion


#pragma region FileDialog
    std::atomic<bool>                                           m_dialogRunning{ false };
    std::mutex                                                  m_dialogResultMutex;
    std::optional<std::filesystem::path>                        m_dialogResult;
    std::function<void(const std::filesystem::path&)>           m_dialogCallback;
    const Asset* m_pendingAsset = nullptr;
    AssetType                                                   m_pendingAssetType = AssetType::UNKNOWN;
    bool                                                        m_pendingIsSave = false;
#pragma endregion
};