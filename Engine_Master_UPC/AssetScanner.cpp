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

AssetScanner::AssetScanner(AssetRegistry* registry) : m_registry(registry)
{
}

std::vector<ImportRequest> AssetScanner::scan(const std::filesystem::path& rootPath)
{
    m_registry->clear();
    m_pendingImports.clear();
    m_knownContentHashes.clear();

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

    // Raw source file — ensure its .meta sidecar exists.
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
        DEBUG_ERROR("[AssetScanner] Failed to load metadata '%s'.", metadataPath.string().c_str());
        return;
    }

    meta.sourcePath = sourcePath.lexically_normal();
    m_registry->registerAsset(meta);

    // Register sub-assets — they share the parent binary so there is
    // nothing extra to check on disk; they are valid iff the parent binary is.
    for (const DependencyRecord& dep : meta.m_dependencies)
    {
        if (dep.localId == INVALID_UID) continue;

        Metadata subMeta;
        subMeta.fileId = dep.localId;
        subMeta.type = dep.type;
        subMeta.contentHash = meta.contentHash;
        m_registry->registerAsset(subMeta);
    }

    // Mark the parent's hash as known so cleanOrphanedBinaries leaves it alone.
    if (!meta.contentHash.empty())
    {
        m_knownContentHashes.insert(meta.contentHash);
    }

    // Reimport needed if:
    //   a) Binary is missing from Library/
    //   b) Source file has changed since last import (hashes differ)
    const MD5Hash currentHash = computeMD5(sourcePath);
    const bool binaryMissing = !FileIO::exists(getBinaryPath(meta.contentHash));
    const bool contentChanged = (currentHash != meta.contentHash);

    if ((binaryMissing || contentChanged) && !isQueued(meta.fileId))
    {
        m_pendingImports.push_back({ sourcePath, meta.fileId });
    }
}

void AssetScanner::handleMissingMetadata(const std::filesystem::path& sourcePath)
{
    if (app->getModuleAssets()->findImporter(sourcePath))
    {
        // INVALID_UID signals ModuleAssets to generate a fresh stable UID.
        m_pendingImports.push_back({ sourcePath, INVALID_UID });
    }
}

void AssetScanner::handleOrphanedMetadata(const std::filesystem::path& metadataPath)
{
    Metadata meta;
    if (app->getModuleAssets()->loadMetadata(metadataPath, meta))
    {
        // One binary covers the parent and ALL its sub-assets — single removal.
        FileIO::remove(getBinaryPath(meta.contentHash));
    }
    FileIO::remove(metadataPath);
}

void AssetScanner::cleanOrphanedBinaries()
{
    if (!FileIO::exists(LIBRARY_FOLDER))
    {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(LIBRARY_FOLDER))
    {
        if (!entry.is_regular_file()) continue;

        // Binary filename IS the contentHash — check against what we collected.
        const std::string stem = entry.path().stem().string();
        if (m_knownContentHashes.count(stem) > 0)
            continue;

        DEBUG_WARN("[AssetScanner] Deleting orphaned binary '%s'.",
            entry.path().string().c_str());
        FileIO::remove(entry.path());
    }
}

bool AssetScanner::isQueued(const UID uid) const
{
    for (const auto& req : m_pendingImports)
    {
        if (req.existingUID == uid) return true;
    }
    return false;
}

std::filesystem::path AssetScanner::getBinaryPath(const MD5Hash& contentHash) const
{
    return std::filesystem::path(LIBRARY_FOLDER) / contentHash += ASSET_EXTENSION;
}