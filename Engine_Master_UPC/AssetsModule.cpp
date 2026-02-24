#include "Globals.h"
#include "AssetsModule.h"

#include "Application.h"
#include "FileSystemModule.h"


#include <filesystem>
#include <fstream>
#include <Logger.h>
#include <UID.h>

UID AssetsModule::find(const std::filesystem::path& assetsFile) const
{
    std::filesystem::path metadataPath = assetsFile;
    metadataPath += METADATA_EXTENSION;

    auto entry = app->getFileSystemModule()->getEntry(metadataPath);
    if (!entry)
    {
        LOG_WARNING("[AssetsModule] Could not find asset '{}'.", assetsFile.string());
        return INVALID_ASSET_ID;
    }

    return entry->uid;
}

UID AssetsModule::import(const std::filesystem::path& assetsFile, UID uid)
{

    Importer* importer = app->getFileSystemModule()->findImporter(assetsFile);
    if (!importer)
    {
        LOG_INFO("[AssetsModule] Couldn't find a proper importer for this format:", assetsFile.c_str());
        return INVALID_ASSET_ID;
    }

    if (uid == INVALID_ASSET_ID) {
        uid = GenerateUID();
    }

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

    std::filesystem::path metaPath = assetsFile;
    metaPath += METADATA_EXTENSION;

    AssetMetadata::saveMetaFile(meta, metaPath);

    // FILE
    uint8_t* buffer = nullptr;
    uint64_t size = importer->save(asset, &buffer);

    app->getFileSystemModule()->save(meta.getBinaryPath(), buffer, static_cast<unsigned int>(size));

    delete buffer;
    delete asset;

    return uid;
}


Asset* AssetsModule::requestAsset(UID id)
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

    return requestAsset(metadata);
}

Asset* AssetsModule::requestAsset(const AssetMetadata* metadata)
{
    if (!metadata)
    {
        LOG_ERROR("[AssetsModule] Couldn't retrieve the metadata with id:", metadata->uid);
        return nullptr;
    }

    //Create the asset
    Importer* importer = app->getFileSystemModule()->findImporter(metadata->type);
    Asset* asset = importer->createAssetInstance(metadata->uid);

    //Load from binary
    char* rawBuffer = nullptr;

    unsigned int size = app->getFileSystemModule()->load(metadata->getBinaryPath(), &rawBuffer);
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
