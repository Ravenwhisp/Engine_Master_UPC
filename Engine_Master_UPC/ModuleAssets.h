#pragma once
#include "Module.h"
#include "UID.h"
#include "AssetsDictionary.h"
#include "WeakCache.h"
#include "AssetRegistry.h"
#include "AssetReference.h"

#include "AssetScanner.h"
#include "ContentRegistry.h"
#include "FileDialogRequest.h"

#include <filesystem>
#include <memory>
#include <Metadata.h>
#include "Importer.h"
#include "FileIO.h"
#include <mutex>

class Asset;
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
private:
    std::unique_ptr<AssetRegistry>                              m_registry;
    std::unique_ptr<AssetScanner>                               m_scanner;
    std::unique_ptr<ContentRegistry>                            m_contentRegistry;
    WeakCache<AssetReference, Asset>                            m_assets;
    std::unordered_map<UID, std::vector<DependencyRecord>>      m_pendingDependencies;

#pragma region Importers
    ImporterTexture* m_importerTexture = nullptr;
    ImporterMesh* m_importerMesh = nullptr;
    ImporterMaterial* m_importerMaterial = nullptr;
    ImporterPrefab* m_importerPrefab = nullptr;
    ImporterAnimation* m_importerAnimation = nullptr;
    ImporterSkin* m_importerSkin = nullptr;
    ImporterGltf* m_importerGltf = nullptr;
    ImporterFont* m_importerFont = nullptr;
    ImporterAnimationStateMachine* m_importerAnimationStateMachine = nullptr;

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

public:
    ~ModuleAssets();

#pragma region GameLoop
    bool init() override;
    bool cleanUp() override;
#pragma endregion

    bool canImport(const std::filesystem::path& sourcePath) const;
    void importAsset(const std::filesystem::path& sourcePath, UID& uid);
    void refresh();

    bool save(const Asset& asset, const std::filesystem::path& path = {});
    bool loadMetadata(const std::filesystem::path& path, Metadata& out);

    UID                         findUID(const std::filesystem::path& sourcePath) const;
    AssetRegistry* getRegistry() { return m_registry.get(); }
    std::shared_ptr<FileEntry>  getRoot() const;
    std::shared_ptr<FileEntry>  getEntry(const std::filesystem::path&) const;
    void                        registerSubAsset(const DependencyRecord& dep, UID parentUID);

#pragma region Importer
    Importer*                   findImporter(const std::filesystem::path& filePath) const;
    Importer*                   findImporter(AssetType type) const;
#pragma endregion

    void flushDialogRequests();

    template<typename T>
    std::shared_ptr<T> load(const AssetReference& ref)
    {
        if (!ref.isValid()) return nullptr;

        // Cache key is the full reference — a mesh and its parent gltf 
        // are different entries even though they share a binary.
        if (auto cached = m_assets.getAs<T>(ref)) return cached;

        const Metadata* meta = m_registry->getMetadata(ref.fileId);
        if (!meta) { return nullptr; }

        const std::vector<uint8_t> buffer = FileIO::read(meta->getBinaryPath());
        Importer* importer = findImporter(meta->type);

        // Importer extracts the right sub-object when localId is valid.
        auto asset = std::shared_ptr<T>(static_cast<T*>(importer->createAssetInstance(ref.isSubAsset() ? ref.localId : ref.fileId)));
        importer->load(buffer.data(), buffer.size(), ref.localId, asset.get());

        m_assets.insert(ref, asset);
        return asset;
    }

private:
    bool persistAsset(const Asset* asset, Importer* importer, const UID& uid, const std::filesystem::path& sourcePath);
    bool isDialogOpen() const { return m_dialogRunning.load(); }

    bool                    writeMetadata(const Metadata& meta, const std::filesystem::path& metaPath);
    void                    requestSave(const Asset& asset);
};