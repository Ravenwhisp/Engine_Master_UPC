#include "Globals.h"
#include "AssetScanner.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "AssetRegistry.h"
#include "Importer.h"
#include "Asset.h"
#include "Metadata.h"
#include "UID.h"
#include "MD5.h"

#include <filesystem>
#include <FileIO.h>

AssetScanner::AssetScanner(AssetRegistry* registry): m_registry(registry)
{
}

std::vector<ImportRequest> AssetScanner::scan(const std::filesystem::path& rootPath)
{
    m_registry->clear();
    m_pendingImports.clear();

    checkFile(rootPath);
    cleanOrphanedBinaries();

    return std::move(m_pendingImports);
}

void AssetScanner::checkFile(const std::filesystem::path& path)
{
    if (FileIO::isDirectory(path))
    {
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            checkFile(entry.path());
        }
        return;
    }

    if (path.extension() == METADATA_EXTENSION)
    {
        loadMetadata(path);
        return;
    }

    // Raw source file — check that its .metadata sidecar exists.
    std::filesystem::path metadataPath = path;
    metadataPath += METADATA_EXTENSION;
    if (!FileIO::exists(metadataPath))
    {
        handleMissingMetadata(path);
    }
}

void AssetScanner::loadMetadata(const std::filesystem::path& metadataPath)
{
    const std::filesystem::path sourcePath = metadataPath.parent_path() / metadataPath.stem();

    if (!FileIO::exists(sourcePath))
    {
        handleOrphanedMetadata(metadataPath);
        return;
    }

    Metadata meta;
    if (!app->getModuleAssets()->loadMetadata(metadataPath, meta))
    {
        DEBUG_ERROR("[AssetScanner] Failed to load metadata '%s'.",  metadataPath.string().c_str());
        return;
    }

    meta.sourcePath = sourcePath.lexically_normal();
    m_registry->registerAsset(meta);

    for (const DependencyRecord& dep : meta.m_dependencies)
    {
        if (!isValidAsset(dep.uid)) continue;

        Metadata subMeta;
        subMeta.uid = dep.uid;
        subMeta.type = dep.type;
        subMeta.m_isSubAsset = true;
        m_registry->registerAsset(subMeta);

        if (!FileIO::exists(getBinaryPath(dep.uid)) && !isQueued(meta.uid))
        {
            m_pendingImports.push_back({ sourcePath, meta.uid });
        }
    }

    // Binary missing for the parent itself — queue for re-import.
    if (!FileIO::exists(getBinaryPath(meta.uid)) && !isQueued(meta.uid))
    {
        m_pendingImports.push_back({ sourcePath, meta.uid });
    }
}

void AssetScanner::handleMissingMetadata(const std::filesystem::path& sourcePath)
{
    if (app->getModuleAssets()->findImporter(sourcePath))
    {
        m_pendingImports.push_back({sourcePath, INVALID_ASSET_ID });
    }
}

void AssetScanner::handleOrphanedMetadata(const std::filesystem::path& metadataPath)
{
    Metadata meta;
    if (app->getModuleAssets()->loadMetadata(metadataPath, meta))
    {
        // Also delete binaries for any sub-assets that were owned by this asset.
        for (const DependencyRecord& dep : meta.m_dependencies)
        {
            FileIO::remove(getBinaryPath(dep.uid));
        }

        FileIO::remove(getBinaryPath(meta.uid));
    }
    FileIO::remove(metadataPath);
}

void AssetScanner::cleanOrphanedBinaries()
{
    if (!FileIO::exists(LIBRARY_FOLDER))
        return;

    for (const auto& entry : std::filesystem::directory_iterator(LIBRARY_FOLDER))
    {
        if (!entry.is_regular_file()) continue;

        const MD5Hash uid = entry.path().stem().string();
        if (m_registry->contains(uid))
            continue;   // known — either a top-level asset or a registered sub-asset

        DEBUG_WARN("[AssetScanner] Deleting orphaned binary '%s' (no metadata).",entry.path().string().c_str());
        FileIO::remove(entry.path());
    }
}

bool AssetScanner::isQueued(const MD5Hash& uid) const
{
    for (const auto& req : m_pendingImports)
    {
        if (req.existingUID == uid) return true;
    }
    return false;
}

std::filesystem::path AssetScanner::getBinaryPath(const MD5Hash& uid) const
{
    return std::filesystem::path(LIBRARY_FOLDER) / uid += ASSET_EXTENSION;
}