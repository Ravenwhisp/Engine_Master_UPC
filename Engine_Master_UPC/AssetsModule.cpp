#include "Globals.h"
#include "AssetsModule.h"

#include "Application.h"
#include "FileSystemModule.h"
#include "Importer.h"

#include <filesystem>
#include <fstream>
#include "UID.h"

UID AssetsModule::import(const std::filesystem::path & assetsFile, UID uid)
{
    Importer* importer = app->getFileSystemModule()->findImporter(assetsFile);
    if (!importer)
    {
        DEBUG_WARN("[AssetsModule] Couldn't find a proper importer for this format:", assetsFile.c_str());
        return INVALID_ASSET_ID;
    }

    if (uid == INVALID_ASSET_ID)
    {
        uid = GenerateUID();
    }

    // Scoped to this function — not cached, not shared.
    std::unique_ptr<Asset> asset(importer->createAssetInstance(uid));

    if (!importer->import(assetsFile, asset.get()))
    {
        DEBUG_ERROR("[AssetsModule] Couldn't import the asset:", assetsFile.c_str());
        return INVALID_ASSET_ID;  // asset cleaned up by unique_ptr
    }

    // Write metadata file alongside the source asset.
    AssetMetadata meta;
    meta.uid = uid;
    meta.type = asset->getType();

    std::filesystem::path metaPath = assetsFile;
    metaPath += METADATA_EXTENSION;

    AssetMetadata::saveMetaFile(meta, metaPath);
    app->getFileSystemModule()->registerMetadata(meta, assetsFile);

    // Serialize and write binary to the library folder.
    uint8_t* rawBuffer = nullptr;
    uint64_t size = importer->save(asset.get(), &rawBuffer);

    // Wrap immediately so it's freed even if save() throws.
    std::unique_ptr<uint8_t[]> buffer(rawBuffer);

    app->getFileSystemModule()->save(
        meta.getBinaryPath(), buffer.get(), static_cast<unsigned int>(size));

    return uid;
}

std::shared_ptr<Asset> AssetsModule::requestAsset(UID id)
{
    auto it = m_assets.find(id);
    if (it != m_assets.end())
    {
        if (auto live = it->second.lock())
        {
            return live;
        }
        // Stale entry — the asset was freed since the last request.
        m_assets.erase(it);
    }

    AssetMetadata* metadata = app->getFileSystemModule()->getMetadata(id);
    if (!metadata)
    {
        DEBUG_ERROR("[AssetsModule] Couldn't retrieve the metadata with id: %d", id);
        return nullptr;
    }

    return loadAsset(metadata);
}

std::shared_ptr<Asset> AssetsModule::loadAsset(const AssetMetadata* metadata)
{
    Importer* importer = app->getFileSystemModule()->findImporter(metadata->type);
    if (!importer)
    {
        DEBUG_ERROR("[AssetsModule] No importer found for asset type of uid: %d", metadata->uid);
        return nullptr;
    }

    std::shared_ptr<Asset> asset(importer->createAssetInstance(metadata->uid));

    char* rawBuffer = nullptr;
    unsigned int size = app->getFileSystemModule()->load(metadata->getBinaryPath(), &rawBuffer);
    if (size > 0)
    {
        std::vector<uint8_t> buffer(rawBuffer, rawBuffer + size);
        importer->load(buffer.data(), asset.get());
        delete[] rawBuffer;
    }

    m_assets[metadata->uid] = asset;

    return asset;
}