#pragma once
#include "Module.h"
#include "UID.h"
#include "AssetsDictionary.h"
#include "WeakCache.h"
#include "AssetRegistry.h"

#include "AssetScanner.h"
#include "ContentRegistry.h"
#include "PrefabSerializer.h"
#include "PrefabAsset.h"

#include <filesystem>
#include <memory>
#include <Metadata.h>

class Asset;
class AnimationStateMachineAsset;
class Importer;
class ImporterTexture;
class ImporterMaterial;
class ImporterMesh;
class ImporterPrefab;
class ImporterAnimation;
class ImporterSkin;
class ImporterAnimationStateMachine;
class ImporterGltf;
class ImporterFont;
struct FileEntry;

// Owns the full asset lifecycle: import, cache, load, unload.
// Also owns the editor content-browser tree.
class ModuleAssets : public Module
{
public:
#pragma region GameLoop
    bool init() override;
    bool cleanUp() override;
#pragma endregion

#pragma region Importer
    Importer* findImporter(const std::filesystem::path& filePath) const;
    Importer* findImporter(AssetType type) const;
    bool canImport(const std::filesystem::path& sourcePath) const;
#pragma endregion

    void importAsset(const std::filesystem::path& sourcePath, UID& uid);

    void refresh();

    // Loads an asset by its stable UID.
    template<typename T>
    std::shared_ptr<T> load(const UID& id)
    {
        if (auto cached = m_assets.getAs<T>(id))
        {
            return cached;
        }

        const Metadata* meta = m_registry->getMetadata(id);
        if (!meta)
        {
            DEBUG_ERROR("[ModuleAssets] No metadata found for UID '%s'.", id.c_str());
            return nullptr;
        }

        return std::static_pointer_cast<T>(loadAsset(meta));
    }

    // Resolves the stable UID from the source path, then delegates to load<T>.
    template<typename T>
    std::shared_ptr<T> loadAtPath(const std::filesystem::path& sourcePath)
    {
        const UID id = m_registry->findByPath(sourcePath);
        if (id == INVALID_UID)
        {
            DEBUG_ERROR("[ModuleAssets] No asset registered at path '%s'.", sourcePath.string().c_str());
            return nullptr;
        }
        return load<T>(id);
    }

    UID  findUID(const std::filesystem::path& sourcePath) const;

    bool isLoaded(const UID& id);
    void unload(const UID& id);

    std::shared_ptr<FileEntry> getRoot()                              const;
    std::shared_ptr<FileEntry> getEntry(const std::filesystem::path&) const;

    bool saveMetaFile(const Metadata& meta, const std::filesystem::path& metaPath);
    bool loadMetaFile(const std::filesystem::path& metaPath, Metadata& outMeta);


    void registerSubAsset(const Metadata& meta, const UID& parentUID,  uint8_t* binaryData, size_t binarySize);

    bool saveAnimationStateMachine(const std::shared_ptr<AnimationStateMachineAsset>& asset);
    bool saveAnimationStateMachineSource(const std::shared_ptr<AnimationStateMachineAsset>& asset);

    bool savePrefab(GameObject* go, const std::filesystem::path& savePath);
    bool applyPrefab(const GameObject* go);
    bool revertPrefab(GameObject* go, Scene* scene);
    bool createVariant(const std::filesystem::path& src, const std::filesystem::path& dst);

    GameObject* spawnPrefab(const PrefabAsset& asset, Scene* scene);
    GameObject* spawnPrefab(const std::filesystem::path& sourcePath, Scene* scene);

private:
    // Loads from disk using the registered importer and inserts into cache.
    std::shared_ptr<Asset> loadAsset(const Metadata* metadata);

    void flushDependencies(const UID& parentUID, const std::filesystem::path& parentSourcePath, AssetType parentType);

    std::unique_ptr<AssetRegistry>      m_registry;
    std::unique_ptr<AssetScanner>       m_scanner;
    std::unique_ptr<ContentRegistry>    m_contentRegistry;

    // Runtime cache keyed by stable UID.
    WeakCache<UID, Asset>               m_assets;

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

    std::vector<Importer*> m_importers;
#pragma endregion


    std::unordered_map<UID, std::vector<DependencyRecord>> m_pendingDependencies;
};