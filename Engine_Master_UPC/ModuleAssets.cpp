#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "ModuleFileSystem.h"
#include "Importer.h"
#include "UID.h"
#include "Delegates.h"

#include <filesystem>
#include <AssetScanner.h>

bool ModuleAssets::init()
{
    m_importHandle = app->getModuleFileSystem()->subscribeToImportRequested(OnImportRequestedEvent::DelegateT::CreateRaw(this, &ModuleAssets::onImportRequested));
    return true;
}

bool ModuleAssets::cleanUp()
{
    app->getModuleFileSystem()->unsubscribeFromImportRequested(m_importHandle);

    m_assets.clear();
    return true;
}

void ModuleAssets::onImportRequested(const ImportRequest& request)
{
    import(request.sourcePath, request.existingUID);
}


UID ModuleAssets::import(const std::filesystem::path & assetsFile, UID uid)
{
    Importer* importer = app->getModuleFileSystem()->findImporter(assetsFile);
    if (!importer)
    {
        DEBUG_WARN("[AssetsModule] No importer found for '{}'.", assetsFile.string());
        return INVALID_ASSET_ID;
    }

    if (uid == INVALID_ASSET_ID)
        uid = GenerateUID();

    // Scoped to this function — not cached, not shared.
    std::unique_ptr<Asset> asset(importer->createAssetInstance(uid));

    if (!importer->import(assetsFile, asset.get()))
    {
        DEBUG_ERROR("[AssetsModule] Import failed for '{}'.", assetsFile.string());
        return INVALID_ASSET_ID;
    }

    // Write the .metadata sidecar alongside the source file.
    AssetMetadata meta;
    meta.uid = uid;
    meta.type = asset->getType();

    std::filesystem::path metaPath = assetsFile;
    metaPath += METADATA_EXTENSION;

    AssetMetadata::saveMetaFile(meta, metaPath);
    app->getModuleFileSystem()->registerMetadata(meta, assetsFile);

    // Serialise the processed binary into the library folder.
    uint8_t* rawBuffer = nullptr;
    uint64_t size = importer->save(asset.get(), &rawBuffer);

    std::unique_ptr<uint8_t[]> buffer(rawBuffer);
    app->getModuleFileSystem()->save(meta.getBinaryPath(), buffer.get(), static_cast<unsigned int>(size));

    return uid;
}

std::shared_ptr<Asset> ModuleAssets::requestAsset(UID id)
{
    if (auto live = m_assets.get(id))
        return live;

    AssetMetadata* metadata = app->getModuleFileSystem()->getMetadata(id);
    if (!metadata)
    {
        DEBUG_ERROR("[AssetsModule] No metadata found for UID: %llu", id);
        return nullptr;
    }

    return loadAsset(metadata);
}

std::shared_ptr<Asset> ModuleAssets::loadAsset(const AssetMetadata* metadata)
{
    Importer* importer = app->getModuleFileSystem()->findImporter(metadata->type);
    if (!importer)
    {
        DEBUG_ERROR("[AssetsModule] No importer found for asset type (UID: %llu).", metadata->uid);
        return nullptr;
    }

    std::shared_ptr<Asset> asset(importer->createAssetInstance(metadata->uid));

    char* rawBuffer = nullptr;
    unsigned int size = app->getModuleFileSystem()->load(metadata->getBinaryPath(), &rawBuffer);

    if (size > 0)
    {
        std::vector<uint8_t> buffer(rawBuffer, rawBuffer + size);
        importer->load(buffer.data(), asset.get());
        delete[] rawBuffer;
    }

    m_assets.insert(metadata->uid, asset);
    return asset;
}