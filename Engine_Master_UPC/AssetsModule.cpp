#include "Globals.h"
#include "AssetsModule.h"

#include "Application.h"
#include "FileSystemModule.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <filesystem>
#include <fstream>
#include <Logger.h>

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
    if (!file.is_open())
    {
        LOG_ERROR("[AssetsModule] Couldn't open the metafile.");
        return false;
    }

    file << buffer.GetString();
    return true;
}

int AssetsModule::import(const std::filesystem::path& assetsFile)
{
    Importer* importer = app->getFileSystemModule()->findImporter(assetsFile);
    if (!importer)
    {
        LOG_INFO("[AssetsModule] Couldn't find a proper importer for this format:", assetsFile.c_str());
        return INVALID_ASSET_ID;
    }

    int uid = generateNewUID();

    Asset* asset = importer->createAssetInstance(uid);

    if (!importer->import(assetsFile, asset))
    {
        LOG_ERROR("[AssetsModule] Couldn't import the asset:", assetsFile.c_str());
        delete asset;
        return INVALID_ASSET_ID;
    }

    std::filesystem::path libraryPath = LIBRARY_FOLDER + std::to_string(uid) + ASSET_EXTENSION;

    // METADATA
    AssetMetadata meta;
    meta.uid = uid;
    meta.type = asset->getType();
    meta.sourcePath = assetsFile;
    meta.binaryPath = libraryPath;

    std::filesystem::path metaPath = assetsFile;
    metaPath += METADATA_EXTENSION;

    saveMetaFile(meta, metaPath);

    // FILE
    uint8_t* buffer = nullptr;
    uint64_t size = importer->save(asset, &buffer);

    app->getFileSystemModule()->save(meta.binaryPath, buffer, static_cast<unsigned int>(size));

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
    if (!metadata)
    {
        LOG_ERROR("[AssetsModule] Couldn't retrieve the metadata with id:", id);
        return nullptr;
    }

    //Create the asset
    Importer* importer = app->getFileSystemModule()->findImporter(metadata->type);
    Asset* asset = importer->createAssetInstance(metadata->uid);

    //Load from binary
    char* rawBuffer = nullptr;

    unsigned int size = app->getFileSystemModule()->load(metadata->binaryPath, &rawBuffer);
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
    if (!asset)
    {
        LOG_WARNING("[AssetsModule] Tried to release an empty asset");
        return;
    }

    asset->release();
    if (asset->getReferenceCount() <= 0)
    {
        m_assets.erase(asset->getId());
        delete asset;
    }
}
