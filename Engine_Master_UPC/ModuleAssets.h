#pragma once
#include "Module.h"
#include "AssetIndex.h"
#include "ImporterRegistry.h"
#include "AssetCache.h"
#include "AssetFileDialog.h"
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

class ModuleAssets : public Module
{
public:
    ModuleAssets();
    ~ModuleAssets();

    bool init() override;
    void postRender() override;
    bool cleanUp() override;

    // ── LOAD ──
    template<typename T>
    std::shared_ptr<T> load(AssetReference& ref);

    template<typename T>
    std::shared_ptr<T> loadAtPath(const std::filesystem::path& sourcePath);

    bool isLoaded(const AssetReference& id);
    void unload(const AssetReference& id);

    // ── SAVE ──
    bool save(Asset& asset, const std::filesystem::path& path = {});

    // ── IMPORT ──
    void importAsset(const std::filesystem::path& sourcePath, AssetReference& reference);
    bool canImport(const std::filesystem::path& sourcePath) const;
    void registerSubAsset(const Metadata& meta, const UID& parentUID,
                          uint8_t* binaryData, size_t binarySize);

    // ── DELETE ──
    void unregisterAsset(const std::filesystem::path& sourcePath);

    // ── Editor ──
    ContentRegistry* getContentRegistry() const;
    PrefabManager*  getPrefabManager() const;
    bool createStateMachineFromGltf(const std::filesystem::path& gltfPath);

    // ── Internal access ──
    AssetIndex& getIndex()           { return m_index; }
    ImporterRegistry& getImporters() { return m_importers; }

    AssetReference* findReference(const UID& uid);

    void refresh();

private:
    bool persistAsset(Asset* asset, Importer* importer, AssetReference& reference,
                      const std::filesystem::path& sourcePath);

    AssetIndex                           m_index;
    ImporterRegistry                     m_importers;
    AssetCache                           m_cache;
    AssetFileDialog                      m_dialog;

    std::unique_ptr<AssetScanner>        m_scanner;
    std::unique_ptr<ContentRegistry>     m_contentRegistry;
    std::unique_ptr<PrefabManager>       m_prefabManager;

    std::unordered_map<UID, std::vector<DependencyRecord>> m_pendingDependencies;
};

#include "ModuleAssets.inl"
