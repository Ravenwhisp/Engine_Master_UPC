#include "Globals.h"

#include "Application.h"
#include "FileSystemModule.h"
#include <filesystem>
#include <fstream>

#include "TextureImporter.h"
#include "ModelImporter.h"
#include "FontImporter.h"
#include "Asset.h"

#include "TextureAsset.h"
#include "AssetsModule.h"

#include "FileIO.h"
#include "ImporterRegistry.h"
#include "MetadataStore.h"

#include "UID.h"

bool FileSystemModule::init()
{
    m_fileIO = new FileIO();
    m_importerRegistry = new ImporterRegistry();
    m_metadataStore = new MetadataStore();

    rebuild();

    return true;
}

AssetMetadata* FileSystemModule::getMetadata(UID uid)
{
    return m_metadataStore->getMetadata(uid);
}

unsigned int FileSystemModule::load(const std::filesystem::path& filePath, char** buffer) const
{
    return m_fileIO->load(filePath, buffer);
}

unsigned int FileSystemModule::load(const char* filePath, char** buffer) const
{
    return m_fileIO->load(filePath, buffer);
}

unsigned int FileSystemModule::save(const std::filesystem::path& filePath, const void* buffer, unsigned int size, bool append) const
{
    return m_fileIO->save(filePath, buffer, size, append);
}

unsigned int FileSystemModule::save(const char* filePath, const void* buffer, unsigned int size, bool append) const
{
    return m_fileIO->save(filePath, buffer, size, append);
}

bool FileSystemModule::copy(const char* sourceFilePath, const char* destinationFilePath) const
{
    return m_fileIO->copy(sourceFilePath, destinationFilePath);
}

bool FileSystemModule::move(const char* sourceFilePath, const char* destinationFilePath) const
{
    return m_fileIO->move(sourceFilePath, destinationFilePath);
}

bool FileSystemModule::deleteFile(const char* filePath) const
{
    return m_fileIO->deleteFile(filePath);
}

bool FileSystemModule::createDirectory(const char* directoryPath) const
{
	return m_fileIO->createDirectory(directoryPath);
}

bool FileSystemModule::exists(const char* filePath) const
{
	return m_fileIO->exists(filePath);
}

bool FileSystemModule::isDirectory(const char* path) const
{
	return m_fileIO->isDirectory(path);
}

Importer* FileSystemModule::findImporter(const std::filesystem::path& filePath) const
{
    return m_importerRegistry->findImporter(filePath);
}

Importer* FileSystemModule::findImporter(const char* filePath) const
{
    return m_importerRegistry->findImporter(filePath);
}

Importer* FileSystemModule::findImporter(AssetType type) const
{
    return m_importerRegistry->findImporter(type);
}

void FileSystemModule::rebuild()
{
    std::string s = ASSETS_FOLDER;
    if (!s.empty() && s.back() == '/')
        s.pop_back();

    m_metadataMap.clear();
    m_pathIndex.clear();
    m_pendingImports.clear();

    checkFile(s);

    for (const auto& pending : m_pendingImports)
    {
        if (pending.existingUID == INVALID_ASSET_ID)
        {
            std::filesystem::path metaPath = pending.sourcePath;
            metaPath += METADATA_EXTENSION;

            if (exists(metaPath.string().c_str()))
            {
                if (m_pathIndex.find(pending.sourcePath.string()) == m_pathIndex.end())
                {
                    loadMetadata(metaPath);
                }

                continue;
            }
        }
        app->getAssetModule()->import(pending.sourcePath, pending.existingUID);
    }

    cleanOrphanedBinaries();
    m_root = buildTree(s);
}

UID FileSystemModule::findByPath(const std::filesystem::path& sourcePath) const
{
    auto it = m_pathIndex.find(sourcePath.lexically_normal().string());
    if (it != m_pathIndex.end())
    {
        return it->second;
    }
    return INVALID_ASSET_ID;
}

void FileSystemModule::registerMetadata(const AssetMetadata& meta, const std::filesystem::path& sourcePath)
{
    m_metadataMap[meta.uid] = meta;
    m_pathIndex[sourcePath.lexically_normal().string()] = meta.uid;
}

std::shared_ptr<FileEntry> FileSystemModule::getEntry(const std::filesystem::path& path)
{
    if (!m_root)
    {
        DEBUG_ERROR("[FileSystemModule] Root folder doesn't exist.");
        return nullptr;
    }
    return getEntryRecursive(m_root, path);
}

std::shared_ptr<FileEntry> FileSystemModule::getEntryRecursive(const std::shared_ptr<FileEntry>& node, const std::filesystem::path& path) const
{
    if (!node)
    {
        //LOG_WARNING("[FileSystemModule] Node doesn't exist");
        return nullptr;
    }

    if (node->path == path)
    {
        return node;
    }

    for (auto& child : node->children)
    {
        if (auto found = getEntryRecursive(child, path))
        {
            return found;
        }
    }

    return nullptr;
}

std::filesystem::path FileSystemModule::getBinaryPath(UID uid) const
{
    return std::filesystem::path(LIBRARY_FOLDER) / std::to_string(uid) += ".asset";
}

void FileSystemModule::handleOrphanedMetadata(const std::filesystem::path& metadataPath)
{
    AssetMetadata meta;
    if (AssetMetadata::loadMetaFile(metadataPath, meta))
    {
        std::filesystem::remove(getBinaryPath(meta.uid));
    }

    std::filesystem::remove(metadataPath);
}

void FileSystemModule::handleMissingMetadata(const std::filesystem::path& sourcePath)
{
    Importer* importer = findImporter(sourcePath);
    if (importer)
    {
        m_pendingImports.push_back({ sourcePath, INVALID_ASSET_ID });
    }
}

std::shared_ptr<FileEntry> FileSystemModule::buildMetadataEntry(const std::filesystem::path& path)
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
        DEBUG_ERROR("[FileSystemModule] Failed to create metadata file node '{}'.", path.string());
    }

    return entry;
}

void FileSystemModule::checkFile(const std::filesystem::path& path)
{
    if (isDirectory(path.string().c_str()))
    {
        for (const auto& p : std::filesystem::directory_iterator(path))
        {
            checkFile(p.path());
        }
        return;
    }

    if (path.extension() == METADATA_EXTENSION)
    {
        loadMetadata(path);
        return;
    }

    // Raw source file — ensure its .metadata exists
    std::filesystem::path metadataPath = path;
    metadataPath += METADATA_EXTENSION;
    if (!exists(metadataPath.string().c_str()))
    {
        handleMissingMetadata(path);
    }
}

void FileSystemModule::loadMetadata(const std::filesystem::path& path)
{
    std::filesystem::path sourcePath = path.parent_path() / path.stem();

    if (!exists(sourcePath.string().c_str()))
    {
        handleOrphanedMetadata(path);
        return;
    }

    AssetMetadata meta;
    if (AssetMetadata::loadMetaFile(path, meta))
    {
        registerMetadata(meta, sourcePath);

        if (!exists(getBinaryPath(meta.uid).string().c_str()))
        {
            m_pendingImports.push_back({ sourcePath, meta.uid });
        }
    }
    else
    {
        DEBUG_ERROR("[FileSystemModule] Failed to load metadata file '{}'.", path.string());
    }
}

std::shared_ptr<FileEntry> FileSystemModule::buildDirectoryEntry(const std::filesystem::path& path)
{
    auto entry = std::make_shared<FileEntry>();
    entry->path = path.lexically_normal();
    entry->isDirectory = true;
    entry->displayName = path.filename().string();

    for (const auto& p : std::filesystem::directory_iterator(path))
    {
        auto child = buildTree(p.path());
        if (child)
        {
            entry->children.push_back(std::move(child));
        }
    }

    return entry;
}

std::shared_ptr<FileEntry> FileSystemModule::buildTree(const std::filesystem::path& path)
{
    if (isDirectory(path.string().c_str()))
    {
        return buildDirectoryEntry(path);
    }

    if (path.extension() == METADATA_EXTENSION)
    {
        return buildMetadataEntry(path);
    }

    return nullptr;
}

void FileSystemModule::cleanOrphanedBinaries()
{
    if (!std::filesystem::exists(LIBRARY_FOLDER))
    {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(LIBRARY_FOLDER))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        std::string stem = entry.path().stem().string();
        UID uid = std::stoull(stem);

        if (m_metadataMap.find(uid) == m_metadataMap.end())
        {
            DEBUG_ERROR("[FileSystemModule] Deleting orphaned binary '{}' with no associated metadata.", entry.path().string());
            std::filesystem::remove(entry.path());
        }
    }
}

