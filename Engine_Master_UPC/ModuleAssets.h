#pragma once
#include "Module.h"
#include "UID.h"
#include "AssetsDictionary.h"
#include "WeakCache.h"
#include "AssetRegistry.h"

#include "ImporterRegistry.h"
#include "AssetScanner.h"
#include "ContentRegistry.h"

#include <filesystem>
#include <memory>
#include <Metadata.h>

class Asset;
class ImporterMaterial;
class ImporterMesh;
class ImporterPrefab;
struct FileEntry;

// Owns the full asset lifecycle: import, cache, load, unload.
// Also owns the editor content-browser tree.
class ModuleAssets : public Module
{
public:
    bool init()    override;
    bool cleanUp() override;
    bool canImport(const std::filesystem::path& sourcePath) const;

    void importAsset(const std::filesystem::path& sourcePath, MD5Hash& uid);

    void refresh();

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

    // Resolves the UID from the source path, then delegates to load<T>.
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


    MD5Hash  findUID(const std::filesystem::path& sourcePath) const;
    bool isLoaded(const MD5Hash& id);
    void unload(const MD5Hash& id);


    std::shared_ptr<FileEntry> getRoot()                              const;
    std::shared_ptr<FileEntry> getEntry(const std::filesystem::path&) const;

    bool saveMetaFile(const Metadata& meta, const std::filesystem::path& metaPath);
    bool loadMetaFile(const std::filesystem::path& metaPath, Metadata& outMeta);

    void registerSubAsset(const Metadata& meta, const MD5Hash& parentUID,
        uint8_t* binaryData, size_t binarySize);
private:
    // Loads from disk using the registered importer and inserts into cache.
    std::shared_ptr<Asset> loadAsset(const Metadata* metadata);

    void flushDependencies(const MD5Hash& parentUID,
        const std::filesystem::path& parentSourcePath,
        AssetType parentType);

    std::unique_ptr<AssetRegistry>      m_registry;
    std::unique_ptr<ImporterRegistry>   m_importerRegistry;
    std::unique_ptr<AssetScanner>       m_scanner;
    std::unique_ptr<ContentRegistry>    m_contentRegistry;
    WeakCache<MD5Hash, Asset>           m_assets;

    ImporterMesh*       m_importerMesh = nullptr;
    ImporterMaterial*   m_importerMaterial = nullptr;
    ImporterPrefab*     m_importerPrefab = nullptr;

    std::unordered_map<MD5Hash, std::vector<DependencyRecord>> m_pendingDependencies;
};