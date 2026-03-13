#pragma once
#include "Delegates.h"
#include "Asset.h"
#include <filesystem>
#include <vector>

class FileIO;
class MetadataStore;
class ImporterRegistry;

// Describes a single asset that needs to be imported or re-imported.
struct ImportRequest
{
    std::filesystem::path sourcePath;
    UID                   existingUID = INVALID_ASSET_ID;
};

DECLARE_EVENT(OnImportRequestedEvent, AssetScanner, const ImportRequest&)

// Scans the assets folder and drives the metadata/binary lifecycle
class AssetScanner
{
public:
    AssetScanner(FileIO* fileIO, MetadataStore* metadataStore, ImporterRegistry* importerRegistry);

    // Clears the metadata store, walks rootPath, resolves all metadata/binary
    // inconsistencies, then fires OnImportRequested for anything that needs work.
    void scan(const std::filesystem::path& rootPath);

    // Subscribers (e.g. AssetsModule) bind to this in their init().
    OnImportRequestedEvent OnImportRequested;

    void unsubscribe(DelegateHandle& handle)
    {
        OnImportRequested.Remove(handle);
    }

private:
    void checkFile(const std::filesystem::path& path);
    void loadMetadata(const std::filesystem::path& metadataPath);
    void handleMissingMetadata(const std::filesystem::path& sourcePath);
    void handleOrphanedMetadata(const std::filesystem::path& metadataPath);
    void cleanOrphanedBinaries();
    void dispatchPendingImports();

    std::filesystem::path getBinaryPath(UID uid) const;

    FileIO* m_fileIO{ nullptr };
    MetadataStore* m_metadataStore{ nullptr };
    ImporterRegistry* m_importerRegistry{ nullptr };

    std::vector<ImportRequest> m_pendingImports;
};