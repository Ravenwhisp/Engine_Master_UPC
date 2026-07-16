#pragma once
#include "Module.h"
#include "AssetIndex.h"
#include "ImporterRegistry.h"
#include "AssetCache.h"
#ifndef GAME_RELEASE
#include "AssetFileDialog.h"
#endif
#include "AssetReference.h"
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

class AssetScanner;
class ContentRegistry;
class PrefabManager;
struct DependencyRecord;
struct ScanFileResult;

struct Metadata;
class DataContainer;

class ModuleAssets : public Module
{
public:
    ModuleAssets();
    ~ModuleAssets();

    bool init() override;
    void postRender() override;
    bool cleanUp() override;

#pragma region Load
    template<typename T>
    std::shared_ptr<T> load(AssetId& ref);

    bool isLoaded(const AssetId& id);
    void unload(const AssetId& id);
#pragma endregion

#pragma region Save
    bool save(Asset& asset, const std::filesystem::path& path = {});
#pragma endregion

#pragma region Import
    void importAsset(const std::filesystem::path& sourcePath, AssetId& reference);
    bool canImport(const std::filesystem::path& sourcePath) const;
    void registerSubAsset(const Metadata& meta, const UID& parentUID, uint8_t* binaryData, size_t binarySize);
#pragma endregion

#pragma region Delete
    void unregisterAsset(const std::filesystem::path& sourcePath);
#pragma endregion

#pragma region Editor
    ContentRegistry* getContentRegistry() const;
    PrefabManager*  getPrefabManager() const;
    bool createStateMachineFromGltf(const std::filesystem::path& gltfPath);
#pragma endregion

    AssetIndex& getIndex()           { return m_index; }
    ImporterRegistry& getImporters() { return m_importers; }

    AssetId* findReference(const UID& uid);

    void refresh();

private:
    bool persistAsset(Asset* asset, Importer* importer, AssetId& reference, const std::filesystem::path& sourcePath);
    DataContainer* resolveDataContainerType(DataContainer* baseContainer) const;

    AssetIndex                           m_index;
    ImporterRegistry                     m_importers;
    AssetCache                           m_cache;
#ifndef GAME_RELEASE
    AssetFileDialog                      m_dialog;

    std::unique_ptr<AssetScanner>        m_scanner;
    std::unique_ptr<ContentRegistry>     m_contentRegistry;
#endif
    std::unique_ptr<PrefabManager>       m_prefabManager;

    std::unordered_map<UID, std::vector<DependencyRecord>> m_pendingDependencies;
};

#include "ModuleAssets.inl"
