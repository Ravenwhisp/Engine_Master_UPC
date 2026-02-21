#include "Globals.h"
#include "AssetsModule.h"

#include "Application.h"
#include "FileSystemModule.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <filesystem>
#include <fstream>

int AssetsModule::find(const std::filesystem::path& assetsFile) const
{
    //We should search fot the same path but with the .metadata
    return 0;
}

bool saveMetaFile(const AssetMetadata& meta, const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();

    auto& alloc = doc.GetAllocator();

    doc.AddMember("uid", rapidjson::Value(meta.uid), alloc);
    doc.AddMember("type", rapidjson::Value(static_cast<int>(meta.type)), alloc);
    doc.AddMember("source", rapidjson::Value(meta.sourcePath.string().c_str(), alloc), alloc);
    doc.AddMember("binary", rapidjson::Value(meta.binaryPath.string().c_str(), alloc), alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(metaPath);
    if (!file.is_open()) return false;

    file << buffer.GetString();
    return true;
}

int AssetsModule::import(const std::filesystem::path& assetsFile)
{
    std::string pathStr = assetsFile.string();
    const char* cpath = pathStr.c_str();

    Importer* importer = app->getFileSystemModule()->findImporter(cpath);
    if (!importer) return INVALID_ASSET_ID;

    int uid = generateNewUID();

    Asset* asset = importer->createAssetInstance(uid);

    if (!importer->import(assetsFile, asset))
    {
        delete asset;
        return INVALID_ASSET_ID;
    }

    std::filesystem::path libraryPath = "Library/" + std::to_string(uid) + ".asset";

    // METADATA
    AssetMetadata meta;
    meta.uid = uid;
    meta.type = asset->getType();
    meta.sourcePath = assetsFile;
    meta.binaryPath = libraryPath;

    std::filesystem::path metaPath = assetsFile;
    metaPath += ".meta";

    saveMetaFile(meta, metaPath);

    // FILE
    uint8_t* buffer = nullptr;
    uint64_t size = importer->save(asset, &buffer);

    std::string librarypathStr = libraryPath.string();
    const char* clibraryPath = librarypathStr.c_str();

    app->getFileSystemModule()->save(clibraryPath, buffer, static_cast<unsigned int>(size));

    delete buffer;
    delete asset;

    //This should maybe be substitute it with a dirty flag?
    app->getFileSystemModule()->rebuild();

    return uid;
}

int AssetsModule::generateNewUID()
{
    return rand();
}

Asset* AssetsModule::requestAsset(int id)
{
    auto it = m_assets.find(id);
    if (it != m_assets.end())
    {
        it->second->addReference();
        return it->second;
    }

    // Load meta
    AssetMetadata* metadata = app->getFileSystemModule()->getMetadata(id);
    if (!metadata) return nullptr;

    //Create the asset
    Importer* importer = app->getFileSystemModule()->findImporter(metadata->type);
    Asset* asset = importer->createAssetInstance(metadata->uid);

    //Load from binary
    char* rawBuffer = nullptr;
    std::string pathStr = metadata->binaryPath.string();
    const char* cpath = pathStr.c_str();

    unsigned int size = app->getFileSystemModule()->load(cpath, &rawBuffer);
    if (size > 0)
    {
        std::vector<uint8_t> buffer(rawBuffer, rawBuffer + size);
        importer->load(buffer.data(), asset);
        delete[] rawBuffer;
    }

    //Store it in map
    asset->addReference();
    m_assets[metadata->uid] = asset;

    return asset;
}

void AssetsModule::releaseAsset(Asset* asset)
{
    if (!asset) return;

    asset->release();
    if (asset->getReferenceCount() <= 0)
    {
        m_assets.erase(asset->getId());
        delete asset;
    }
}
