#pragma once

#include "AssetCache.h"
#include "Asset.h"
#include "AssetIndex.h"
#include "ImporterRegistry.h"
#include "Importer.h"
#include "Metadata.h"
#include "FileIO.h"
#include "AssetsDictionary.h"
#include "JsonArchive.h"
#include <cstdint>
#include <vector>

template<typename T>
std::shared_ptr<T> AssetCache::loadFromLibrary(AssetReference& ref, ImporterRegistry& importers,
                                                AssetIndex& index)
{
    if (ref.m_type == AssetType::UNKNOWN)
    {
        return nullptr;
    }

    Importer* importer = importers.findByType(ref.m_type);
    if (!importer)
    {
        DEBUG_ERROR("[AssetCache] No importer for type %u (UID '%s').",
            static_cast<unsigned>(ref.m_type), std::to_string(ref.m_uid).c_str());
        return nullptr;
    }

    const std::filesystem::path binaryPath = std::filesystem::path(LIBRARY_FOLDER) / ref.m_libId += ASSET_EXTENSION;

    const std::vector<uint8_t> buffer = FileIO::read(binaryPath);
    if (buffer.empty())
    {
        return nullptr;
    }

    std::shared_ptr<Asset> asset(importer->createAssetInstance(ref));
    importer->load(buffer.data(), asset.get());

#ifndef GAME_RELEASE
    const AssetIndexEntry* entry = index.findEntry(ref.m_uid);
    if (entry && !entry->sourcePath.empty())
    {
        std::filesystem::path metaPath = entry->sourcePath;
        Metadata::getMetadataPath(metaPath);
        Metadata meta;
        JsonArchive metaArchive(ArchiveMode::Input);
        if (metaArchive.loadFile(metaPath))
        {
            meta.serialize(metaArchive);
            if (meta.importSettings)
            {
                asset->setImportSettings(std::move(meta.importSettings));
            }
        }

        asset->setImportSettings(std::move(meta.importSettings));
    }
#endif
    if (!asset->getImportSettings())
    {
        asset->setImportSettings(asset->createDefaultImportSettings());
    }

    m_cache.insert(ref.m_uid, asset);

    return std::static_pointer_cast<T>(asset);
}


template<typename T>
std::shared_ptr<T> AssetCache::load(AssetReference& ref, AssetIndex& index, ImporterRegistry& importers)
{
    if (!isValidUID(ref.m_uid))
    {
        return nullptr;
    }

    if (auto cached = m_cache.getAs<T>(ref.m_uid))
    {
        return cached;
    }

    if (isValidAsset(ref.m_libId))
    {
        if (ref.m_type == AssetType::UNKNOWN)
        {
            const AssetIndexEntry* entry = index.findEntry(ref.m_uid);
            if (entry)
            {
                ref.m_type = entry->type;
            }
        }

        {
            const AssetIndexEntry* entry = index.findEntry(ref.m_uid);
            if (entry && isValidAsset(entry->contentHash))
            {
                ref.m_libId = entry->contentHash;
            }
        }

        if (auto loaded = loadFromLibrary<T>(ref, importers, index))
        {
            return loaded;
        }
    }

    const AssetIndexEntry* entry = index.findEntry(ref.m_uid);
    if (!entry || entry->sourcePath.empty())
    {
        DEBUG_ERROR("[AssetCache] Cannot load UID '%s': no source path available for re-import.",
            std::to_string(ref.m_uid).c_str());
        return nullptr;
    }

    return nullptr;
}


template<typename T>
std::shared_ptr<T> AssetCache::loadAtPath(const std::filesystem::path& sourcePath,
                                           AssetIndex& index, ImporterRegistry& importers)
{
    const UID uid = index.findUID(sourcePath);
    if (isValidUID(uid))
    {
        if (auto cached = m_cache.getAs<T>(uid))
        {
            return cached;
        }
    }

    std::filesystem::path metaPath = sourcePath;
    Metadata::getMetadataPath(metaPath);

    Metadata meta;
    JsonArchive metaArchive(ArchiveMode::Input);
    if (metaArchive.loadFile(metaPath))
    {
        meta.serialize(metaArchive);
        index.registerEntry(meta.uid, meta.type, sourcePath);
        AssetReference ref(meta.uid, meta.contentHash, meta.type);
        return load<T>(ref, index, importers);
    }

    return nullptr;
}
