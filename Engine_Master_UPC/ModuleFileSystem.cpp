#include "Globals.h"
#include "ModuleFileSystem.h"
#include <filesystem>
#include <fstream>

#include "TextureImporter.h"
#include "ModelImporter.h"
#include "FontImporter.h"
#include "Asset.h"

#include "TextureAsset.h"
#include "ModuleAssets.h"

bool ModuleFileSystem::init()
{

    auto textureImporter = new TextureImporter();
    auto modelImporter = new ModelImporter();
    auto fontImporter = new FontImporter();

    importersMap.emplace(AssetType::TEXTURE, textureImporter);
    importersMap.emplace(AssetType::MODEL, modelImporter);
    importersMap.emplace(AssetType::FONT, fontImporter);

    importers.push_back(textureImporter);
    importers.push_back(modelImporter);
    importers.push_back(fontImporter);

    rebuild();

    return true;
}

Importer* ModuleFileSystem::findImporter(const std::filesystem::path& filePath)
{
    std::string pathStr = filePath.string();
    const char* cpath = pathStr.c_str();
    return findImporter(cpath);
}

Importer* ModuleFileSystem::findImporter(const char* filePath)
{
    for (auto importer : importers) 
    {
        if (importer->canImport(filePath))
        {
            return importer;
        }
    }
    return nullptr;
}

Importer* ModuleFileSystem::findImporter(AssetType type)
{
    auto it = importersMap.find(type);
    if (it != importersMap.end())
    {
        return it->second;
    }

    return nullptr;
}

AssetMetadata* ModuleFileSystem::getMetadata(UID uid)
{
    auto it = m_metadataMap.find(uid);
    if (it != m_metadataMap.end())
    {
        return &it->second;
    }

    return nullptr;
}

unsigned int ModuleFileSystem::load(const std::filesystem::path& filePath, char** buffer) const
{
    std::string pathStr = filePath.string();
    const char* cpath = pathStr.c_str();
    return load(cpath, buffer);
}

unsigned int ModuleFileSystem::load(const char* filePath, char** buffer) const
{
    if (!filePath || !buffer)
    {
        DEBUG_ERROR("[ModuleFileSystem] No path or buffer correctly provided.");
        return 0;
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) 
    { 
        DEBUG_ERROR("[ModuleFileSystem] Couldn't open the file while trying to load.");
        return 0; 
    }

    std::streamsize size = file.tellg();
    if (size <= 0)
    {
        DEBUG_ERROR("[ModuleFileSystem] Couldn't load the file since it's empty.");
        return 0;
    }

    file.seekg(0, std::ios::beg);

    char* data = new char[size];
    if (!file.read(data, size))
    {
        DEBUG_ERROR("[ModuleFileSystem] Couldn't read the file.");
        delete[] data;
        return 0;
    }

    *buffer = data;
    return static_cast<unsigned int>(size);
}

unsigned int ModuleFileSystem::save(const std::filesystem::path& filePath, const void* buffer, unsigned int size, bool append) const
{
    std::string pathStr = filePath.string();
    const char* cpath = pathStr.c_str();
    return save(cpath, buffer, size, append);
}

unsigned int ModuleFileSystem::save(const char* filePath, const void* buffer, unsigned int size, bool append) const
{
    if (!filePath || !buffer || size == 0) return 0;

    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    std::ios::openmode mode = std::ios::binary | std::ios::out;
    if (append)
    {
        mode |= std::ios::app;
    }
    else
    {
        mode |= std::ios::trunc;

    }

    std::ofstream file(filePath, mode);
    if (!file)
    {
        DEBUG_ERROR("[ModuleFileSystem] Error while trying to save a file that doesn't exists.");
        return 0;
    }

    file.write(static_cast<const char*>(buffer), size);
    if (!file)
    {
        DEBUG_ERROR("[ModuleFileSystem] Error while writing into a file.");
        return 0;
    }

    return size;
}

bool ModuleFileSystem::copy(const char* sourceFilePath, const char* destinationFilePath) const
{
	return std::filesystem::copy_file(sourceFilePath, destinationFilePath);
}

bool ModuleFileSystem::move(const char* sourceFilePath, const char* destinationFilePath) const
{
    std::error_code error;
    std::filesystem::rename(sourceFilePath, destinationFilePath, error);
    return error.value() == 0;
}

bool ModuleFileSystem::deleteFile(const char* filePath) const
{
	return std::filesystem::remove(filePath);
}

bool ModuleFileSystem::createDirectory(const char* directoryPath) const
{
	return std::filesystem::create_directory(directoryPath);
}

bool ModuleFileSystem::exists(const char* filePath) const
{
	return std::filesystem::exists(filePath);
}

bool ModuleFileSystem::isDirectory(const char* path) const
{
	return std::filesystem::is_directory(path);
}

void ModuleFileSystem::rebuild()
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
        app->getAssetModule()->importAsset(pending.sourcePath, pending.existingUID);
    }

    cleanOrphanedBinaries();
    m_root = buildTree(s);
}

UID ModuleFileSystem::findByPath(const std::filesystem::path& sourcePath) const
{
    auto it = m_pathIndex.find(sourcePath.lexically_normal().string());
    if (it != m_pathIndex.end())
    {
        return it->second;
    }
    return INVALID_ASSET_ID;
}

void ModuleFileSystem::registerMetadata(const AssetMetadata& meta, const std::filesystem::path& sourcePath)
{
    m_metadataMap[meta.uid] = meta;
    m_pathIndex[sourcePath.lexically_normal().string()] = meta.uid;
}

std::shared_ptr<FileEntry> ModuleFileSystem::getEntry(const std::filesystem::path& path)
{
    if (!m_root)
    {
        DEBUG_ERROR("[ModuleFileSystem] Root folder doesn't exist.");
        return nullptr;
    }
    return getEntryRecursive(m_root, path);
}

std::shared_ptr<FileEntry> ModuleFileSystem::getEntryRecursive(const std::shared_ptr<FileEntry>& node, const std::filesystem::path& path) const
{
    if (!node)
    {
        //LOG_WARNING("[ModuleFileSystem] Node doesn't exist");
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

std::filesystem::path ModuleFileSystem::getBinaryPath(UID uid) const
{
    return std::filesystem::path(LIBRARY_FOLDER) / std::to_string(uid) += ".asset";
}

void ModuleFileSystem::handleOrphanedMetadata(const std::filesystem::path& metadataPath)
{
    AssetMetadata meta;
    if (AssetMetadata::loadMetaFile(metadataPath, meta))
    {
        std::filesystem::remove(getBinaryPath(meta.uid));
    }

    std::filesystem::remove(metadataPath);
}

void ModuleFileSystem::handleMissingMetadata(const std::filesystem::path& sourcePath)
{
    Importer* importer = findImporter(sourcePath);
    if (importer)
    {
        m_pendingImports.push_back({ sourcePath, INVALID_ASSET_ID });
    }
}

std::shared_ptr<FileEntry> ModuleFileSystem::buildMetadataEntry(const std::filesystem::path& path)
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
        DEBUG_ERROR("[ModuleFileSystem] Failed to create metadata file node '{}'.", path.string());
    }

    return entry;
}

void ModuleFileSystem::checkFile(const std::filesystem::path& path)
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

    if (path.extension() == ".prefab") return;

    // Raw source file — ensure its .metadata exists
    std::filesystem::path metadataPath = path;
    metadataPath += METADATA_EXTENSION;
    if (!exists(metadataPath.string().c_str()))
    {
        handleMissingMetadata(path);
    }
}

void ModuleFileSystem::loadMetadata(const std::filesystem::path& path)
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
        DEBUG_ERROR("[ModuleFileSystem] Failed to load metadata file '{}'.", path.string());
    }
}

std::shared_ptr<FileEntry> ModuleFileSystem::buildDirectoryEntry(const std::filesystem::path& path)
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

std::shared_ptr<FileEntry> ModuleFileSystem::buildTree(const std::filesystem::path& path)
{
    if (isDirectory(path.string().c_str()))
    {
        return buildDirectoryEntry(path);
    }

    if (path.extension() == METADATA_EXTENSION)
    {
        return buildMetadataEntry(path);
    }

    if (path.extension() == ".prefab")
    {
        auto entry = std::make_shared<FileEntry>();
        entry->path = path.lexically_normal();
        entry->isDirectory = false;
        entry->displayName = path.stem().string();
        entry->uid = INVALID_ASSET_ID;
        return entry;
    }


    return nullptr;
}

void ModuleFileSystem::cleanOrphanedBinaries()
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
            DEBUG_ERROR("[ModuleFileSystem] Deleting orphaned binary '{}' with no associated metadata.", entry.path().string());
            std::filesystem::remove(entry.path());
        }
    }
}

