#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "Asset.h"

// A node in the asset browser tree.
// Directories hold children; file nodes carry the UID of their asset.
struct FileEntry
{
    std::filesystem::path                   path;
    std::string                             displayName;
    UID                                     uid = INVALID_ASSET_ID;
    bool                                    isDirectory = false;
    std::vector<std::shared_ptr<FileEntry>> children;

    std::filesystem::path getPath() const
    {
        if (isDirectory)
        {
            std::filesystem::path p = path;
            p += "/";
            return p;
        }
        return path;
    }
};

class ModuleFileSystem;
class AssetRegistry;

// Owns and rebuilds the in-memory FileEntry tree that the editor's
// content browser reads. UIDs are resolved from MetadataStore (in memory)
// rather than re-reading .metadata files from disk.
class ContentRegistry
{
public:
    // Both pointers must outlive this object.
    ContentRegistry(ModuleFileSystem* fs, AssetRegistry* registry);

    void rebuild(const std::filesystem::path& rootPath);

    std::shared_ptr<FileEntry> getRoot()                                const { return m_root; }
    std::shared_ptr<FileEntry> getEntry(const std::filesystem::path&)   const;

private:
    std::shared_ptr<FileEntry> buildTree(const std::filesystem::path& path)             const;
    std::shared_ptr<FileEntry> buildDirectoryEntry(const std::filesystem::path& path)   const;
    std::shared_ptr<FileEntry> buildAssetEntry(const std::filesystem::path& metaPath)   const;

    std::shared_ptr<FileEntry> getEntryRecursive(
        const std::shared_ptr<FileEntry>& node,
        const std::filesystem::path& path) const;

    ModuleFileSystem* m_fs{ nullptr };
    AssetRegistry* m_registry{ nullptr };

    std::shared_ptr<FileEntry> m_root;
};