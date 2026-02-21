#include "Globals.h"
#include "FileSystemModule.h"
#include <filesystem>
#include <fstream>
#include <simdjson.h>
#include <Logger.h>

#include "TextureImporter.h"
#include "ModelImporter.h"
#include "Asset.h"

#include <TextureAsset.h>

bool FileSystemModule::init()
{
    rebuild();

    /// TESTING
    auto textureImporter = new TextureImporter();
    auto modelImporter = new ModelImporter();

    TextureAsset skyBox(rand());
    textureImporter->import(L"Assets/Textures/cubemap2.dds", &skyBox);

    uint8_t* buffer = nullptr;
    uint64_t size = textureImporter->save(&skyBox, &buffer);
    save("Library/Textures/cubemap2.asset", buffer, static_cast<unsigned int>(size));

    textureImporter->load(buffer, &skyBox);
    ///

    importersMap.emplace(AssetType::TEXTURE, textureImporter);
    importersMap.emplace(AssetType::MODEL, modelImporter);

    importers.push_back(textureImporter);
    importers.push_back(modelImporter);

    return true;
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

AssetMetadata* FileSystemModule::getMetadata(int uid)
{
    auto it = m_metadataMap.find(uid);
    if (it != m_metadataMap.end())
    {
        return &it->second;
    }

    return nullptr;
}

unsigned int FileSystemModule::load(const char* filePath, char** buffer) const
{
    if (!filePath || !buffer) return 0;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) return 0;

    std::streamsize size = file.tellg();
    if (size <= 0) return 0;

    file.seekg(0, std::ios::beg);

    char* data = new char[size];
    if (!file.read(data, size))
    {
        delete[] data;
        return 0;
    }

    *buffer = data;
    return static_cast<unsigned int>(size);
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
    if (!file) return 0;

    file.write(static_cast<const char*>(buffer), size);
    if (!file) return 0;

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
    m_root = buildTree("Assets");
}

std::shared_ptr<FileEntry> FileSystemModule::getEntry(const std::filesystem::path& path)
{
    if (!m_root) return nullptr;
    return getEntryRecursive(m_root, path);
}

std::shared_ptr<FileEntry> FileSystemModule::getEntryRecursive( const std::shared_ptr<FileEntry>& node, const std::filesystem::path& path) const
{
    if (!node) return nullptr;

    if (node->path == path) return node;

    for (auto& child : node->children)
    {
        if (auto found = getEntryRecursive(child, path)) return found;
    }

    return nullptr;
}


bool loadMetaFile(const std::filesystem::path& metaPath, AssetMetadata& outMeta)
{
    try
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json = simdjson::padded_string::load(metaPath.string());

        auto doc = parser.iterate(json);

        outMeta.uid = doc["uid"].get_uint64().value();
        outMeta.type = static_cast<AssetType>(doc["type"].get_uint64().value());

        outMeta.sourcePath = std::string(doc["source"].get_string().value());
        outMeta.binaryPath = std::string(doc["binary"].get_string().value());

        return true;
    }
    catch (const simdjson::simdjson_error& e)
    {
        LOG_ERROR("Failed to load meta file '%s': %s", metaPath.string().c_str(), e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Unexpected error loading meta file '%s': %s", metaPath.string().c_str(), e.what());
        return false;
    }
}

std::shared_ptr<FileEntry> FileSystemModule::buildTree(const std::filesystem::path& path)
{
    auto entry = std::make_shared<FileEntry>();
    entry->path = path;
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
        if (path.extension() == ".meta")
        {
            AssetMetadata meta;
            if (loadMetaFile(path, meta))
            {
                m_metadataMap[meta.uid] = meta;
            }
        }
    }

    return entry;
}

