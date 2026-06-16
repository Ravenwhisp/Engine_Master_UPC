#pragma once

#include "Metadata.h"
#include "UID.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class AssetIndex;

struct AssetEntry
{
    UID uid = INVALID_UID;
    std::string displayName;
    Metadata metadata;
    std::vector<AssetEntry> subAssets;
};

struct DirectoryEntry
{
    std::filesystem::path path;
    DirectoryEntry* parent = nullptr;
    std::string displayName;
    Metadata metadata;

    std::vector<std::unique_ptr<DirectoryEntry>> directories;
    std::vector<AssetEntry> assets;
};

class ContentRegistry
{
public:
    ContentRegistry();

    void rebuild(const std::filesystem::path& rootPath, const AssetIndex* index);
    void registerAsset(const std::filesystem::path& sourcePath, const AssetIndex* index);

    void unregisterAsset(const std::filesystem::path& sourcePath);

    DirectoryEntry* getRoot() const;
    DirectoryEntry* getDirectory(const std::filesystem::path& path) const;

private:
    std::unique_ptr<DirectoryEntry> buildDirectory(const std::filesystem::path& path,
                                                    DirectoryEntry* parent,
                                                    const AssetIndex* index) const;

    void addAsset(DirectoryEntry& directory, const std::filesystem::path& metaPath,
                  const AssetIndex* index) const;

    DirectoryEntry* findDirectoryRecursive(DirectoryEntry* directory,
                                            const std::filesystem::path& path) const;

private:
    std::unique_ptr<DirectoryEntry> m_root;
};
