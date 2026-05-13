#include "Globals.h"
#include "ContentRegistry.h"

#include "FileIO.h"
#include "AssetsDictionary.h"
#include "ModuleAssets.h"

#include <filesystem>

namespace fs = std::filesystem;

ContentRegistry::ContentRegistry(ModuleAssets* moduleAssets) : m_moduleAssets(moduleAssets)
{
}

void ContentRegistry::rebuild(const fs::path& rootPath)
{
    m_root = buildDirectory(rootPath.lexically_normal(), nullptr);
}

DirectoryEntry* ContentRegistry::getRoot() const
{
    return m_root.get();
}

DirectoryEntry* ContentRegistry::getDirectory(const fs::path& path) const
{
    if (!m_root)
    {
        return nullptr;
    }

    return findDirectoryRecursive(m_root.get(), path.lexically_normal());
}

std::unique_ptr<DirectoryEntry> ContentRegistry::buildDirectory(const fs::path& path, DirectoryEntry* parent) const
{
    auto directory = std::make_unique<DirectoryEntry>();

    directory->path = path.lexically_normal();
    directory->parent = parent;
    directory->displayName = directory->path.filename().string();

    for (const auto& entry : fs::directory_iterator(path))
    {
        const fs::path entryPath = entry.path().lexically_normal();

        if (FileIO::isDirectory(entryPath))
        {
            directory->directories.push_back(buildDirectory(entryPath, directory.get()));
        }
        else if (entryPath.extension() == METADATA_EXTENSION)
        {
            addAsset(*directory, entryPath);
        }
    }

    return directory;
}

void ContentRegistry::addAsset(DirectoryEntry& directory, const fs::path& metaPath) const
{
    fs::path sourcePath = metaPath;
    sourcePath.replace_extension();

    AssetEntry asset;
    asset.displayName = sourcePath.filename().string();

    asset.uid = m_moduleAssets->findUID(sourcePath.lexically_normal().string());

    directory.assets.push_back(asset);
}

DirectoryEntry* ContentRegistry::findDirectoryRecursive(DirectoryEntry* directory, const fs::path& path) const
{
    if (!directory)
    {
        return nullptr;
    }

    if (directory->path.lexically_normal() == path.lexically_normal())
    {
        return directory;
    }

    for (const auto& child : directory->directories)
    {
        if (DirectoryEntry* found = findDirectoryRecursive(child.get(), path))
        {
            return found;
        }
    }

    return nullptr;
}