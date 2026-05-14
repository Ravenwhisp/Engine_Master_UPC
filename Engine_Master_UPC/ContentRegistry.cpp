#include "Globals.h"
#include "ContentRegistry.h"
#include "AssetRegistry.h"
#include "Asset.h"
#include <FileIO.h>

ContentRegistry::ContentRegistry(AssetRegistry* registry):
    m_registry(registry)
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
    if (FileIO::isDirectory(path))
    {
        return buildDirectoryEntry(path);
    }


    if (path.extension() == METADATA_EXTENSION)
    {
        return buildAssetEntry(path);
    }

    // Raw source files (.png, .fbx, …) are not shown in the browser.
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

std::shared_ptr<FileEntry> ContentRegistry::buildAssetEntry(const std::filesystem::path& metaPath) const
{
    auto entry = std::make_shared<FileEntry>();
    entry->path = metaPath.lexically_normal();
    entry->isDirectory = false;
    entry->displayName = metaPath.stem().string();  // strips .metadata → shows asset name

    // Resolve UID from the in-memory store — no disk read needed here
    // because rebuild() is always called after AssetScanner::scan().
    const std::filesystem::path sourcePath = metaPath.parent_path() / metaPath.stem();
    entry->uid = m_registry->findByPath(sourcePath);

    if (entry->uid == INVALID_ASSET_ID)
    {
        DEBUG_WARN("[ContentRegistry] No metadata in store for '%s'. Was rebuild() called before scan()?", sourcePath.string().c_str());
    }


    return entry;
}

std::shared_ptr<FileEntry> ContentRegistry::getEntryRecursive(
    const std::shared_ptr<FileEntry>& node,
    const std::filesystem::path& path) const
{
    if (!node)
        return nullptr;

    if (node->path == path)
        return node;

    for (const auto& child : node->children)
    {
        if (auto found = getEntryRecursive(child, path))
            return found;
    }

    return nullptr;
}