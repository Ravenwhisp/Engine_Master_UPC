#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "ModuleFileSystem.h"
#include "Importer.h"
#include "Asset.h"
#include "UID.h"

#include "TextureImporter.h"
#include "ModelImporter.h"
#include "FontImporter.h"

#include <filesystem>


bool ModuleAssets::init()
{
    m_registry = std::make_unique<AssetRegistry>();
    m_importerRegistry = std::make_unique<ImporterRegistry>();

    m_importerRegistry->registerImporter(std::make_unique<TextureImporter>());
    m_importerRegistry->registerImporter(std::make_unique<ModelImporter>());
    m_importerRegistry->registerImporter(std::make_unique<FontImporter>());

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


UID ModuleAssets::importAsset(const std::filesystem::path& sourcePath, UID uid)
{
    Importer* importer = m_importerRegistry->findImporter(sourcePath);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] No importer found for '%s'.", sourcePath.string().c_str());
        return INVALID_ASSET_ID;
    }

    if (uid == INVALID_ASSET_ID)
    {
        uid = GenerateUID();
    }

    // Scoped to this function — not cached, not shared.
    std::unique_ptr<Asset> asset(importer->createAssetInstance(uid));

    if (!importer->import(sourcePath, asset.get()))
    {
        DEBUG_ERROR("[ModuleAssets] Import failed for '%s'.", sourcePath.string().c_str());
        return INVALID_ASSET_ID;
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
        return INVALID_ASSET_ID;
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
        return INVALID_ASSET_ID;
    }

    return uid;
}

void ModuleAssets::refresh()
{
    std::string rootStr = ASSETS_FOLDER;
    if (!rootStr.empty() && (rootStr.back() == '/' || rootStr.back() == '\\'))
    {
        rootStr.pop_back();
    }

    const std::filesystem::path root = rootStr;

    const std::vector<ImportRequest> pending = m_scanner->scan(root);
    for (const ImportRequest& req : pending)
    {
        importAsset(req.sourcePath, req.existingUID);
    }

    m_contentRegistry->rebuild(root);
}


UID ModuleAssets::findUID(const std::filesystem::path& sourcePath) const
{
    return m_registry->findByPath(sourcePath);
}

bool ModuleAssets::isLoaded(UID id)
{
    return m_assets.contains(id);
}

void ModuleAssets::unload(UID id)
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