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
    std::filesystem::path              path;
    std::string                        displayName;
    UID                                uid = INVALID_ASSET_ID;
    bool                               isDirectory = false;
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

class FileIO;

// Owns and rebuilds the in-memory FileEntry tree that the editor's
// content browser reads.
class ContentRegistry
{
public:
    explicit ContentRegistry(FileIO* fileIO);

    void rebuild(const std::filesystem::path& rootPath);
    std::shared_ptr<FileEntry> getRoot() const { return m_root; }
    std::shared_ptr<FileEntry> getEntry(const std::filesystem::path& path) const;

private:

    std::shared_ptr<FileEntry> buildTree(const std::filesystem::path& path) const;
    std::shared_ptr<FileEntry> buildDirectoryEntry(const std::filesystem::path& path) const;
    std::shared_ptr<FileEntry> buildMetadataEntry(const std::filesystem::path& path) const;

    std::shared_ptr<FileEntry> getEntryRecursive( const std::shared_ptr<FileEntry>& node, const std::filesystem::path& path) const;

    FileIO* m_fileIO{ nullptr };
    std::shared_ptr<FileEntry> m_root;
};