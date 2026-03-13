#include "Globals.h"
#include "AssetScanner.h"

#include "FileIO.h"
#include "MetadataStore.h"
#include "ImporterRegistry.h"
#include "Importer.h"
#include "Asset.h"
#include "UID.h"

#include <filesystem>

AssetScanner::AssetScanner(FileIO* fileIO, MetadataStore* metadataStore, ImporterRegistry* importerRegistry)
    : m_fileIO(fileIO)
    , m_metadataStore(metadataStore)
    , m_importerRegistry(importerRegistry)
{
}

void AssetScanner::scan(const std::filesystem::path& rootPath)
{
    m_metadataStore->clear();
    m_pendingImports.clear();

    checkFile(rootPath);

    cleanOrphanedBinaries();
    dispatchPendingImports();
}


void AssetScanner::checkFile(const std::filesystem::path& path)
{
    if (m_fileIO->isDirectory(path.string().c_str()))
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
    if (!m_fileIO->exists(metadataPath.string().c_str()))
    {
        handleMissingMetadata(path);
    }
}

void AssetScanner::loadMetadata(const std::filesystem::path& metadataPath)
{
    std::filesystem::path sourcePath = metadataPath.parent_path() / metadataPath.stem();

    if (!m_fileIO->exists(sourcePath.string().c_str()))
    {
        handleOrphanedMetadata(metadataPath);
        return;
    }

    AssetMetadata meta;
    if (!AssetMetadata::loadMetaFile(metadataPath, meta))
    {
        DEBUG_ERROR("[AssetScanner] Failed to load metadata file '{}'.", metadataPath.string());
        return;
    }

    m_metadataStore->registerMetadata(meta, sourcePath);

    // Binary missing → re-import from source.
    if (!m_fileIO->exists(getBinaryPath(meta.uid).string().c_str()))
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
        std::filesystem::remove(getBinaryPath(meta.uid));
    }

    std::filesystem::remove(metadataPath);
}

void AssetScanner::cleanOrphanedBinaries()
{
    if (!m_fileIO->exists(LIBRARY_FOLDER)) return;

    for (const auto& entry : std::filesystem::directory_iterator(LIBRARY_FOLDER))
    {
        if (!entry.is_regular_file()) continue;

        UID uid = std::stoull(entry.path().stem().string());

        if (!m_metadataStore->contains(uid))
        {
            DEBUG_ERROR("[AssetScanner] Deleting orphaned binary '{}' with no associated metadata.",
                entry.path().string());
            m_fileIO->deleteFile(entry.path().string().c_str());
        }
    }
}

void AssetScanner::dispatchPendingImports()
{
    for (const ImportRequest& request : m_pendingImports)
    {
        OnImportRequested.Broadcast(request);
    }

    m_pendingImports.clear();
}

std::filesystem::path AssetScanner::getBinaryPath(UID uid) const
{
    return std::filesystem::path(LIBRARY_FOLDER) / std::to_string(uid) += ".asset";
}