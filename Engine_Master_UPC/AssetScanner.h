#pragma once
#include "Asset.h"
#include <filesystem>
#include <vector>

class ModuleFileSystem;
class AssetRegistry;
class ImporterRegistry;

// Describes a single asset that needs to be imported or re-imported.
struct ImportRequest
{
    std::filesystem::path sourcePath;
    UID                   existingUID = INVALID_ASSET_ID;
};

// Scans the assets folder and resolves metadata / binary lifecycle.
// Returns the list of files that need (re-)importing; the caller decides
// what to do with them — no event bus required.
class AssetScanner
{
public:
    AssetScanner(ModuleFileSystem* fs, AssetRegistry* metadataStore, ImporterRegistry* importerRegistry);

    std::vector<ImportRequest> scan(const std::filesystem::path& rootPath);

private:
    void checkFile(const std::filesystem::path& path);
    void loadMetadata(const std::filesystem::path& metadataPath);
    void handleMissingMetadata(const std::filesystem::path& sourcePath);
    void handleOrphanedMetadata(const std::filesystem::path& metadataPath);
    void cleanOrphanedBinaries();

    std::filesystem::path getBinaryPath(UID uid) const;

    ModuleFileSystem* m_fs{ nullptr };
    AssetRegistry* m_registry{ nullptr };
    ImporterRegistry* m_importerRegistry{ nullptr };

    std::vector<ImportRequest> m_pendingImports;
};