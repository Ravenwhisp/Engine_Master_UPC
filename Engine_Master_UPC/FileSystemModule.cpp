#include "Globals.h"
#include "FileSystemModule.h"
#include <filesystem>
#include <fstream>

#include <Logger.h>

#include "TextureImporter.h"
#include "ModelImporter.h"
#include "Asset.h"

#include <TextureAsset.h>
#include <AssetsModule.h>

bool FileSystemModule::init()
{
    rebuild();

    auto textureImporter = new TextureImporter();
    auto modelImporter = new ModelImporter();

    importersMap.emplace(AssetType::TEXTURE, textureImporter);
    importersMap.emplace(AssetType::MODEL, modelImporter);

    importers.push_back(textureImporter);
    importers.push_back(modelImporter);

    return true;
}

Importer* FileSystemModule::findImporter(const std::filesystem::path& filePath)
{
    std::string pathStr = filePath.string();
    const char* cpath = pathStr.c_str();
    return findImporter(cpath);
}

Importer* FileSystemModule::findImporter(const char* filePath)
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

Importer* FileSystemModule::findImporter(AssetType type)
{
    auto it = importersMap.find(type);
    if (it != importersMap.end())
    {
        return it->second;
    }

    return nullptr;
}

AssetMetadata* FileSystemModule::getMetadata(UID uid)
{
    auto it = m_metadataMap.find(uid);
    if (it != m_metadataMap.end())
    {
        return &it->second;
    }

    return nullptr;
}

unsigned int FileSystemModule::load(const std::filesystem::path& filePath, char** buffer) const
{
    std::string pathStr = filePath.string();
    const char* cpath = pathStr.c_str();
    return load(cpath, buffer);
}

unsigned int FileSystemModule::load(const char* filePath, char** buffer) const
{
    if (!filePath || !buffer)
    {
        LOG_ERROR("[FileSystemModule] No path or buffer correctly provided.");
        return 0;
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) 
    { 
        LOG_ERROR("[FileSystemModule] Couldn't open the file while trying to load.");
        return 0; 
    }

    std::streamsize size = file.tellg();
    if (size <= 0)
    {
        LOG_ERROR("[FileSystemModule] Couldn't load the file since it's empty.");
        return 0;
    }

    file.seekg(0, std::ios::beg);

    char* data = new char[size];
    if (!file.read(data, size))
    {
        LOG_ERROR("[FileSystemModule] Couldn't read the file.");
        delete[] data;
        return 0;
    }

    *buffer = data;
    return static_cast<unsigned int>(size);
}

unsigned int FileSystemModule::save(const std::filesystem::path& filePath, const void* buffer, unsigned int size, bool append) const
{
    std::string pathStr = filePath.string();
    const char* cpath = pathStr.c_str();
    return save(cpath, buffer, size, append);
}

unsigned int FileSystemModule::save(const char* filePath, const void* buffer, unsigned int size, bool append) const
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
        LOG_ERROR("[FileSystemModule] Error while trying to save a file that doesn't exists.");
        return 0;
    }

    file.write(static_cast<const char*>(buffer), size);
    if (!file)
    {
        LOG_ERROR("[FileSystemModule] Error while writing into a file.");
        return 0;
    }

    return size;
}

bool FileSystemModule::copy(const char* sourceFilePath, const char* destinationFilePath) const
{
	return std::filesystem::copy_file(sourceFilePath, destinationFilePath);
}

bool FileSystemModule::deleteFile(const char* filePath) const
{
	return std::filesystem::remove(filePath);
}

bool FileSystemModule::createDirectory(const char* directoryPath) const
{
	return std::filesystem::create_directory(directoryPath);
}

bool FileSystemModule::exists(const char* filePath) const
{
	return std::filesystem::exists(filePath);
}

bool FileSystemModule::isDirectory(const char* path) const
{
	return std::filesystem::is_directory(path);
}

void FileSystemModule::rebuild()
{
    std::string s = ASSETS_FOLDER;
    if (!s.empty() && s.back() == '/')
    {
        s.pop_back();
    }

    m_root = buildTree(s);
}

std::shared_ptr<FileEntry> FileSystemModule::getEntry(const std::filesystem::path& path)
{
    if (!m_root)
    {
        LOG_WARNING("[FileSystemModule] Root folder doesn't exist.");
        return nullptr;
    }
    return getEntryRecursive(m_root, path);
}

std::shared_ptr<FileEntry> FileSystemModule::getEntryRecursive(const std::shared_ptr<FileEntry>& node, const std::filesystem::path& path) const
{
    if (!node)
    {
        //LOG_WARNING("[FileSystemModule] Node doesn't exists");
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

std::shared_ptr<FileEntry> FileSystemModule::buildTree(const std::filesystem::path& path)
{
    auto entry = std::make_shared<FileEntry>();
    entry->path = path.lexically_normal();
    entry->isDirectory = isDirectory(path.string().c_str());

    if (entry->isDirectory)
    {
        for (const auto& p : std::filesystem::directory_iterator(path))
        {
            entry->children.push_back(buildTree(p.path()));
        }
    }
    else
    {
        if (path.extension() == METADATA_EXTENSION)
        {
            AssetMetadata meta;
            if (AssetMetadata::loadMetaFile(path, meta))
            {
                m_metadataMap[meta.uid] = meta;
                return nullptr;
            }
        }
    }

    return entry;
}

