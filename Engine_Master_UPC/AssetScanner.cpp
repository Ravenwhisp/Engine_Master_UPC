#include "Globals.h"
#include "AssetScanner.h"

#include "ModuleFileSystem.h"
#include "AssetRegistry.h"
#include "ImporterRegistry.h"
#include "Importer.h"
#include "Asset.h"
#include "UID.h"

#include <filesystem>

AssetScanner::AssetScanner(ModuleFileSystem* fs, AssetRegistry* registry, ImporterRegistry* importerRegistry)
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
    if (!m_fs->exists(metadataPath))
    {
        handleMissingMetadata(path);
    }
}

void AssetScanner::loadMetadata(const std::filesystem::path& metadataPath)
{
    const std::filesystem::path sourcePath = metadataPath.parent_path() / metadataPath.stem();

    if (!m_fs->exists(sourcePath))
    {
        handleOrphanedMetadata(metadataPath);
        return;
    }

    AssetMetadata meta;
    if (!AssetMetadata::loadMetaFile(metadataPath, meta))
    {
        DEBUG_ERROR("[AssetScanner] Failed to load metadata file '%s'.", metadataPath.string().c_str());
        return;
    }

    meta.sourcePath = sourcePath.lexically_normal();
    m_registry->registerAsset(meta);

    // Binary missing — queue for re-import.
    if (!m_fs->exists(getBinaryPath(meta.uid)))
    {
        m_pendingImports.push_back({ sourcePath, meta.uid });
    }

}

void AssetScanner::handleMissingMetadata(const std::filesystem::path& sourcePath)
{
    // Only queue importable formats — ignore unknown extensions silently.
    if (m_importerRegistry->findImporter(sourcePath))
    {
        m_pendingImports.push_back({ sourcePath, INVALID_ASSET_ID });
    }

}

void AssetScanner::handleOrphanedMetadata(const std::filesystem::path& metadataPath)
{
    AssetMetadata meta;
    if (AssetMetadata::loadMetaFile(metadataPath, meta))
    {
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
        if (!entry.is_regular_file())
            continue;

        const UID uid = std::stoull(entry.path().stem().string());
        if (!m_registry->contains(uid))
        {
            DEBUG_WARN("[AssetScanner] Deleting orphaned binary '%s' (no metadata).",
                entry.path().string().c_str());
            m_fs->remove(entry.path());
        }
    }
}

std::filesystem::path AssetScanner::getBinaryPath(UID uid) const
{
    return std::filesystem::path(LIBRARY_FOLDER) / std::to_string(uid) += ".asset";
}