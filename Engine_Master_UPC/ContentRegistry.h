#pragma once

#include "Metadata.h"
#include "UID.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class ModuleAssets;

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

private:
    ModuleAssets* m_moduleAssets = nullptr;
    std::unique_ptr<DirectoryEntry> m_root;

public:
    ContentRegistry(ModuleAssets* m_moduleAssets);

    void rebuild(const std::filesystem::path& rootPath);
    void registerAsset(const std::filesystem::path& sourcePath);

    void unregisterAsset(const std::filesystem::path& sourcePath);
    void registerDirectory(const std::filesystem::path& dirPath);
    void unregisterDirectory(const std::filesystem::path& dirPath);

    DirectoryEntry* getRoot() const;
    DirectoryEntry* getDirectory(const std::filesystem::path& path) const;

private:
    std::unique_ptr<DirectoryEntry> buildDirectory(const std::filesystem::path& path, DirectoryEntry* parent) const;

    void addAsset(DirectoryEntry& directory, const std::filesystem::path& metaPath) const;

    DirectoryEntry* findDirectoryRecursive(DirectoryEntry* directory, const std::filesystem::path& path) const;
};
