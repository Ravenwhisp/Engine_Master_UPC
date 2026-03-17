#include "Globals.h"
#include "AssetScanner.h"

#include "Application.h"
#include "ModuleFileSystem.h"
#include "ModuleAssets.h"
#include "AssetRegistry.h"
#include "ImporterRegistry.h"
#include "Importer.h"
#include "Asset.h"
#include "Metadata.h"
#include "UID.h"

#include <filesystem>

AssetScanner::AssetScanner(ModuleFileSystem* fs,
    AssetRegistry* registry,
    ImporterRegistry* importerRegistry)
    : m_fs(fs)
    , m_registry(registry)
    , m_importerRegistry(importerRegistry)
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
    if (m_fs->isDirectory(path))
    {
        for (const auto& entry : std::filesystem::directory_iterator(path))
            checkFile(entry.path());
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
    if (!m_fs->exists(metadataPath))
        handleMissingMetadata(path);
}

void AssetScanner::loadMetadata(const std::filesystem::path& metadataPath)
{
    const std::filesystem::path sourcePath =
        metadataPath.parent_path() / metadataPath.stem();

    if (!m_fs->exists(sourcePath))
    {
        handleOrphanedMetadata(metadataPath);
        return;
    }

    Metadata meta;
    if (!app->getModuleAssets()->loadMetaFile(metadataPath, meta))
    {
        DEBUG_ERROR("[AssetScanner] Failed to load metadata '%s'.",
            metadataPath.string().c_str());
        return;
    }

    meta.sourcePath = sourcePath.lexically_normal();
    m_registry->registerAsset(meta);

    // ── Register sub-assets listed in the dependency array ───────────────────
    // Sub-assets have no source file and no .metadata sidecar of their own.
    // Their only on-disk record is this parent's dependency list.
    // Registering them here means the scanner is aware of them for the rest of
    // the session and cleanOrphanedBinaries will not delete their Library/ binaries.
    for (const DependencyRecord& dep : meta.m_dependencies)
    {
        if (!isValidAsset(dep.uid)) continue;

        Metadata subMeta;
        subMeta.uid = dep.uid;
        subMeta.type = dep.type;
        subMeta.m_isSubAsset = true;
        // sourcePath is intentionally empty — sub-assets have no source file.
        m_registry->registerAsset(subMeta);

        // If the sub-asset binary is missing, queue the parent for re-import
        // so the binary is regenerated.
        if (!m_fs->exists(getBinaryPath(dep.uid)))
        {
            // Only add one re-import request per parent, even if several
            // sub-assets are missing.
            bool alreadyQueued = false;
            for (const auto& req : m_pendingImports)
                if (req.existingUID == meta.uid) { alreadyQueued = true; break; }

            if (!alreadyQueued)
                m_pendingImports.push_back({ sourcePath, meta.uid });
        }
    }

    // Binary missing for the parent itself — queue for re-import.
    if (!m_fs->exists(getBinaryPath(meta.uid)))
    {
        bool alreadyQueued = false;
        for (const auto& req : m_pendingImports)
            if (req.existingUID == meta.uid) { alreadyQueued = true; break; }

        if (!alreadyQueued)
            m_pendingImports.push_back({ sourcePath, meta.uid });
    }
}

void AssetScanner::handleMissingMetadata(const std::filesystem::path& sourcePath)
{
    if (m_importerRegistry->findImporter(sourcePath))
        m_pendingImports.push_back({ sourcePath, INVALID_ASSET_ID });
}

void AssetScanner::handleOrphanedMetadata(const std::filesystem::path& metadataPath)
{
    Metadata meta;
    if (app->getModuleAssets()->loadMetaFile(metadataPath, meta))
    {
        // Also delete binaries for any sub-assets that were owned by this asset.
        for (const DependencyRecord& dep : meta.m_dependencies)
            m_fs->remove(getBinaryPath(dep.uid));

        m_fs->remove(getBinaryPath(meta.uid));
    }
    m_fs->remove(metadataPath);
}

void AssetScanner::cleanOrphanedBinaries()
{
    if (!m_fs->exists(LIBRARY_FOLDER))
        return;

    for (const auto& entry : std::filesystem::directory_iterator(LIBRARY_FOLDER))
    {
        if (!entry.is_regular_file()) continue;

        const MD5Hash uid = entry.path().stem().string();
        if (m_registry->contains(uid))
            continue;   // known — either a top-level asset or a registered sub-asset

        DEBUG_WARN("[AssetScanner] Deleting orphaned binary '%s' (no metadata).",
            entry.path().string().c_str());
        m_fs->remove(entry.path());
    }
}

std::filesystem::path AssetScanner::getBinaryPath(const MD5Hash& uid) const
{
    return std::filesystem::path(LIBRARY_FOLDER) / uid += ASSET_EXTENSION;
}