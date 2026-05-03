#pragma once

#include "Asset.h"
#include "Metadata.h"

#include <filesystem>
#include <vector>

class AssetRegistry;
class ThreadPool;

struct ImportRequest
{
    std::filesystem::path sourcePath;
    UID existingUID = INVALID_UID;
};

class AssetScanner
{
public:
    AssetScanner(AssetRegistry* metadataStore);

    std::vector<ImportRequest> scan(const std::filesystem::path& rootPath);

private:
    struct ScanFileResult
    {
        std::vector<Metadata> metadata;
        std::vector<ImportRequest> imports;
    };

private:
    void collectFiles(const std::filesystem::path& path, std::vector<std::filesystem::path>& files) const;

    void checkFile(const std::filesystem::path& path, ScanFileResult& result) const;
    void loadMetadata(const std::filesystem::path& metadataPath, ScanFileResult& result) const;
    void handleMissingMetadata(const std::filesystem::path& sourcePath, ScanFileResult& result) const;

    static void queueImport(
        std::vector<ImportRequest>& imports,
        const std::filesystem::path& sourcePath,
        const UID& existingUID);

private:
    AssetRegistry* m_registry{ nullptr };
    ThreadPool* m_threadPool{ nullptr };
};