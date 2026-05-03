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

AssetScanner::AssetScanner(AssetRegistry* registry)
    : m_registry(registry)
{
}

std::vector<ImportRequest> AssetScanner::scan(const std::filesystem::path& rootPath)
{
    m_registry->clear();
    m_pendingImports.clear();

    checkFile(rootPath);

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
        return;
    }

    Metadata meta;
    if (!app->getModuleAssets()->loadMetaFile(metadataPath, meta))
    {
        DEBUG_ERROR("[AssetScanner] Failed to load metadata '%s'.", metadataPath.string().c_str());
        return;
    }

    meta.sourcePath = sourcePath.lexically_normal();
    m_registry->registerAsset(meta);

    const MD5Hash currentHash = computeMD5(sourcePath);
    const bool contentChanged = !isValidAsset(meta.contentHash) || (meta.contentHash != currentHash);
    const bool binaryMissing = !FileIO::exists(meta.getBinaryPath());

    if (contentChanged || binaryMissing)
    {
        queueImport(sourcePath, meta.uid);
    }

    for (const DependencyRecord& dep : meta.m_dependencies)
    {
        if (!isValidUID(dep.uid)) continue;

        Metadata subMeta;
        subMeta.uid = dep.uid;
        subMeta.contentHash = dep.contentHash;
        subMeta.type = dep.type;
        subMeta.m_isSubAsset = true;

        m_registry->registerAsset(subMeta);

        if (!FileIO::exists(dep.getBinaryPath()))
        {
            queueImport(sourcePath, meta.uid);
        }
    }
}

void AssetScanner::handleMissingMetadata(const std::filesystem::path& sourcePath)
{
    // Brand-new file: no UID has been assigned yet.
    if (app->getModuleAssets()->findImporter(sourcePath))
    {
        m_pendingImports.push_back({ sourcePath, INVALID_UID });
    }
}

void AssetScanner::queueImport(const std::filesystem::path& sourcePath, const UID& existingUID)
{
    for (const auto& req : m_pendingImports)
    {
        if (req.sourcePath == sourcePath)
            return;     // already queued
    }
    m_pendingImports.push_back({ sourcePath, existingUID });
}