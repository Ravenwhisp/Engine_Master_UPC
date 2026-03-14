#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "ModuleFileSystem.h"
#include "Importer.h"
#include "ImporterMesh.h"
#include "ImporterMaterial.h"
#include "ImporterTexture.h"
#include "ImporterPrefab.h"
#include "ImporterGltf.h"
#include "ImporterFont.h"


#include "Asset.h"
#include "UID.h"

#include <filesystem>


bool ModuleAssets::init()
{
    m_registry = std::make_unique<AssetRegistry>();
    m_importerRegistry = std::make_unique<ImporterRegistry>();
    
  /*  m_importerRegistry->registerImporter(std::make_unique<ImporterTexture>());
    m_importerMesh = new ImporterMesh();
    m_importerMaterial = new ImporterMaterial();

    m_importerRegistry->registerImporter(std::make_unique<ImporterMesh>(m_importerMesh));
    m_importerRegistry->registerImporter(std::make_unique<ImporterMaterial>(m_importerMesh));

    m_importerRegistry->registerImporter(std::make_unique<ImporterGltf>(*m_importerMesh, *m_importerMaterial));
    m_importerRegistry->registerImporter(std::make_unique<ImporterPrefab>());
    m_importerRegistry->registerImporter(std::make_unique<ImporterFont>());*/

    ModuleFileSystem* fs = app->getModuleFileSystem();
    m_scanner = std::make_unique<AssetScanner>(fs, m_registry.get(), m_importerRegistry.get());
    m_contentRegistry = std::make_unique<ContentRegistry>(fs, m_registry.get());

    refresh();

    return true;
}

bool ModuleAssets::cleanUp()
{
    m_assets.clear();
    return true;
}

bool ModuleAssets::canImport(const std::filesystem::path& sourcePath) const
{
    return m_importerRegistry->findImporter(sourcePath) != nullptr;
}

void ModuleAssets::importAsset(const std::filesystem::path& sourcePath, MD5Hash& uid)
{
    Importer* importer = m_importerRegistry->findImporter(sourcePath);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] No importer found for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    if (!isValidAsset(uid))
    {
        uid = computeMD5(sourcePath);
    }

    // Scoped to this function — not cached, not shared.
    std::unique_ptr<Asset> asset(importer->createAssetInstance(uid));

    if (!importer->import(sourcePath, asset.get()))
    {
        DEBUG_ERROR("[ModuleAssets] Import failed for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    // Write the .metadata sidecar alongside the source file.
    AssetMetadata meta;
    meta.uid = uid;
    meta.type = asset->getType();

    std::filesystem::path metaPath = sourcePath;
    metaPath += METADATA_EXTENSION;

    if (!AssetMetadata::saveMetaFile(meta, metaPath))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write metadata for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    m_registry->registerAsset(meta);

    // Serialise the processed binary into the library folder.
    uint8_t* rawBuffer = nullptr;
    const uint64_t size = importer->save(asset.get(), &rawBuffer);
    std::unique_ptr<uint8_t[]> buffer(rawBuffer);

    if (!app->getModuleFileSystem()->write(meta.getBinaryPath(), buffer.get(), static_cast<size_t>(size)))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write binary for '%s'.", sourcePath.string().c_str());
        // Metadata was already written — roll back the in-memory store entry
        // so it doesn't reference a binary that doesn't exist.
        m_registry->remove(uid);
        uid = INVALID_ASSET_ID;
        return;
    }

    return;
}

void ModuleAssets::refresh()
{
    std::string rootStr = ASSETS_FOLDER;
    if (!rootStr.empty() && (rootStr.back() == '/' || rootStr.back() == '\\'))
    {
        rootStr.pop_back();
    }

    const std::filesystem::path root = rootStr;

    std::vector<ImportRequest> pending = m_scanner->scan(root);
    for (ImportRequest& req : pending)
    {
        importAsset(req.sourcePath, req.existingUID);
    }

    m_contentRegistry->rebuild(root);
}


MD5Hash ModuleAssets::findUID(const std::filesystem::path& sourcePath) const
{
    return m_registry->findByPath(sourcePath);
}

bool ModuleAssets::isLoaded(const MD5Hash& id)
{
    return m_assets.contains(id);
}

void ModuleAssets::unload(const MD5Hash& id)
{
    m_assets.remove(id);
}


std::shared_ptr<FileEntry> ModuleAssets::getRoot() const
{
    return m_contentRegistry->getRoot();
}

std::shared_ptr<FileEntry> ModuleAssets::getEntry(const std::filesystem::path& path) const
{
    return m_contentRegistry->getEntry(path);
}


std::shared_ptr<Asset> ModuleAssets::loadAsset(const AssetMetadata* metadata)
{
    Importer* importer = m_importerRegistry->findImporter(metadata->type);
    if (!importer)
    {
        DEBUG_ERROR("[ModuleAssets] No importer for asset type %u (UID %llu).", static_cast<unsigned>(metadata->type), metadata->uid);
        return nullptr;
    }

    std::shared_ptr<Asset> asset(importer->createAssetInstance(metadata->uid));

    const std::vector<uint8_t> buffer = app->getModuleFileSystem()->read(metadata->getBinaryPath());
    if (buffer.empty())
    {
        DEBUG_ERROR("[ModuleAssets] Binary missing or empty for UID %llu.", metadata->uid);
        return nullptr;
    }

    importer->load(buffer.data(), asset.get());
    m_assets.insert(metadata->uid, asset);

    return asset;
}

void ModuleAssets::registerSubAsset(const AssetMetadata& meta, uint8_t* binaryData, size_t binarySize)
{
    m_registry->registerAsset(meta);

    if (binaryData && binarySize > 0)
    {
        if (!app->getModuleFileSystem()->write(meta.getBinaryPath(), binaryData, binarySize))
        {
            DEBUG_ERROR("[ModuleAssets] Failed to write binary for sub-asset UID %s.", meta.uid.c_str());
            m_registry->remove(meta.uid);
        }
    }
}