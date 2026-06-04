#pragma once

#include "ModuleAssets.h"
#include "Asset.h"
#include "Metadata.h"
#include "AssetsDictionary.h"
#include "MD5.h"
#include "JsonArchive.h"

template<typename T>
std::shared_ptr<T> ModuleAssets::load(AssetReference& ref)
{
    if (!isValidUID(ref.m_uid))
    {
        return nullptr;
    }

    if (auto cached = m_cache.get(ref.m_uid))
    {
        if (auto typed = std::dynamic_pointer_cast<T>(cached))
        {
            return typed;
        }
    }

    if (isValidAsset(ref.m_libId))
    {
        if (ref.m_type == AssetType::UNKNOWN)
        {
            const AssetIndexEntry* entry = m_index.findEntry(ref.m_uid);
            if (entry)
            {
                ref.m_type = entry->type;
            }
        }

        {
            const AssetIndexEntry* entry = m_index.findEntry(ref.m_uid);
            if (entry && isValidAsset(entry->contentHash))
            {
                ref.m_libId = entry->contentHash;
            }
        }

        if (auto loaded = m_cache.loadFromLibrary<T>(ref, m_importers, m_index))
        {
            return loaded;
        }
    }

    const AssetIndexEntry* entry = m_index.findEntry(ref.m_uid);
    if (!entry || entry->sourcePath.empty())
    {
        DEBUG_ERROR("[ModuleAssets] Cannot load UID '%s': no source path available for re-import.",
            std::to_string(ref.m_uid).c_str());
        return nullptr;
    }

    importAsset(entry->sourcePath, ref);
    if (!isValidAsset(ref.m_libId))
    {
        return nullptr;
    }

    return m_cache.loadFromLibrary<T>(ref, m_importers, m_index);
}

template<typename T>
std::shared_ptr<T> ModuleAssets::loadAtPath(const std::filesystem::path& sourcePath)
{
    const UID uid = m_index.findUID(sourcePath);
    if (isValidUID(uid))
    {
        if (auto cached = m_cache.get(uid))
        {
            if (auto typed = std::dynamic_pointer_cast<T>(cached))
            {
                return typed;
            }
        }
    }

    std::filesystem::path metaPath = sourcePath;
    Metadata::getMetadataPath(metaPath);

    Metadata meta;
    JsonArchive metaArchive(ArchiveMode::Input);
    if (metaArchive.loadFile(metaPath))
    {
        meta.serialize(metaArchive);
        m_index.registerEntry(meta.uid, meta.type, sourcePath);
        AssetReference ref(meta.uid, meta.contentHash, meta.type);
        return load<T>(ref);
    }

    AssetReference ref(isValidUID(uid) ? uid : INVALID_UID);
    importAsset(sourcePath, ref);
    if (!ref.isValid())
    {
        return nullptr;
    }

    return m_cache.loadFromLibrary<T>(ref, m_importers, m_index);
}
