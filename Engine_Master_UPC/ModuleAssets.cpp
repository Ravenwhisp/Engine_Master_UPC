#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "ModuleFileSystem.h"


#include <filesystem>
#include <fstream>
#include "UID.h"

UID ModuleAssets::import(const std::filesystem::path& assetsFile, UID uid)
{

    Importer* importer = app->getModuleFileSystem()->findImporter(assetsFile);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] Couldn't find a proper importer for this format:", assetsFile.c_str());
        return INVALID_ASSET_ID;
    }

    if (uid == INVALID_ASSET_ID) {
        uid = GenerateUID();
    }

    Asset* asset = importer->createAssetInstance(uid);

    if (!importer->import(assetsFile, asset))
    {
        DEBUG_ERROR("[ModuleAssets] Couldn't import the asset:", assetsFile.c_str());
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
    app->getModuleFileSystem()->registerMetadata(meta, assetsFile);
    // FILE
    uint8_t* buffer = nullptr;
    uint64_t size = importer->save(asset, &buffer);

    app->getModuleFileSystem()->save(meta.getBinaryPath(), buffer, static_cast<unsigned int>(size));

    delete buffer;
    delete asset;

    return uid;
}


Asset* ModuleAssets::requestAsset(UID id)
{
    auto it = m_assets.find(id);
    if (it != m_assets.end())
    {
        it->second->addReference();
        return it->second;
    }

    // Load meta
    AssetMetadata* metadata = app->getModuleFileSystem()->getMetadata(id);
    if (!metadata)
    {
        DEBUG_ERROR("[ModuleAssets] Couldn't retrieve the metadata with id: %d", id);
        return nullptr;
    }

    return requestAsset(metadata);
}

Asset* ModuleAssets::requestAsset(const AssetMetadata* metadata)
{
    if (!metadata)
    {
        DEBUG_ERROR("[ModuleAssets] Couldn't retrieve the metadata with id: %d", metadata->uid);
        return nullptr;
    }

    //Create the asset
    Importer* importer = app->getModuleFileSystem()->findImporter(metadata->type);
    Asset* asset = importer->createAssetInstance(metadata->uid);

    //Load from binary
    char* rawBuffer = nullptr;

    unsigned int size = app->getModuleFileSystem()->load(metadata->getBinaryPath(), &rawBuffer);
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

void ModuleAssets::releaseAsset(Asset* asset)
{
    if (!asset)
    {
        DEBUG_WARN("[ModuleAssets] Tried to release an empty asset");
        return;
    }

    asset->release();
    if (asset->getReferenceCount() <= 0)
    {
        m_assets.erase(asset->getId());
        delete asset;
    }
}
