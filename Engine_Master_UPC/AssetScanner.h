#pragma once
#include "Asset.h"
#include <filesystem>
#include <vector>
#include "MD5Fwd.h"

class AssetRegistry;
class ImporterRegistry;

// Describes a single asset that needs to be imported or re-imported.
struct ImportRequest
{
    std::filesystem::path sourcePath;
    UID               existingUID = INVALID_UID;
};

// Scans the assets folder and resolves metadata / binary lifecycle.
// Returns the list of files that need (re-)importing; the caller decides
// what to do with them — no event bus required.
class AssetScanner
{
public:
    AssetScanner(AssetRegistry* metadataStore);

    std::vector<ImportRequest> scan(const std::filesystem::path& rootPath);

private:
    void checkFile(const std::filesystem::path& path);
    void loadMetadata(const std::filesystem::path& metadataPath);
    void handleMissingMetadata(const std::filesystem::path& sourcePath);
    void handleOrphanedMetadata(const std::filesystem::path& metadataPath);
    void cleanOrphanedBinaries();
    bool isQueued(const UID uid) const;

    std::filesystem::path getBinaryPath(const MD5Hash& contentHash) const;

    AssetRegistry* m_registry{ nullptr };

    std::vector<ImportRequest> m_pendingImports;
    std::unordered_set<std::string> m_knownContentHashes;
};