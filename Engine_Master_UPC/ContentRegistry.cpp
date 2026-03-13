#include "Globals.h"
#include "ContentRegistry.h"
#include "FileIO.h"
#include "Asset.h"

#include <filesystem>

ContentRegistry::ContentRegistry(FileIO* fileIO)
    : m_fileIO(fileIO)
{
}

void ContentRegistry::rebuild(const std::filesystem::path& rootPath)
{
    m_root = buildTree(rootPath);
}

std::shared_ptr<FileEntry> ContentRegistry::getEntry(const std::filesystem::path& path) const
{
    if (!m_root)
    {
        DEBUG_ERROR("[ContentRegistry] Tree has not been built yet — call rebuild() first.");
        return nullptr;
    }
    return getEntryRecursive(m_root, path);
}


std::shared_ptr<FileEntry> ContentRegistry::buildTree(const std::filesystem::path& path) const
{
    if (m_fileIO->isDirectory(path.string().c_str()))
    {
        return buildDirectoryEntry(path);
    }


    if (path.extension() == METADATA_EXTENSION)
    {
        return buildMetadataEntry(path);
    }

    // Raw source files (.png, .fbx, …) are not shown in the browser
    return nullptr;
}

std::shared_ptr<FileEntry> ContentRegistry::buildDirectoryEntry(const std::filesystem::path& path) const
{
    auto entry = std::make_shared<FileEntry>();
    entry->path = path.lexically_normal();
    entry->isDirectory = true;
    entry->displayName = entry->path.filename().string();

    for (const auto& p : std::filesystem::directory_iterator(path))
    {
        if (auto child = buildTree(p.path()))
        {
            entry->children.push_back(std::move(child));
        }
    }

    return entry;
}

std::shared_ptr<FileEntry> ContentRegistry::buildMetadataEntry(const std::filesystem::path& path) const
{
    auto entry = std::make_shared<FileEntry>();
    entry->path = path.lexically_normal();
    entry->isDirectory = false;
    entry->displayName = path.stem().string();

    AssetMetadata meta;
    if (AssetMetadata::loadMetaFile(path, meta))
    {
        entry->uid = meta.uid;
    }
    else
    {
        DEBUG_ERROR("[ContentRegistry] Failed to read metadata for tree node '{}'.", path.string());
    }

    return entry;
}

std::shared_ptr<FileEntry> ContentRegistry::getEntryRecursive(
    const std::shared_ptr<FileEntry>& node,
    const std::filesystem::path& path) const
{
    if (!node)
    {
        return nullptr;
    }

    if (node->path == path)
    {
        return node;
    }


    for (const auto& child : node->children)
    {
        if (auto found = getEntryRecursive(child, path))
        {
            return found;
        }
    }

    return nullptr;
}