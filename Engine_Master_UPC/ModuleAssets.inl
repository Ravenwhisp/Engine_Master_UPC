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

    {
        const AssetIndexEntry* entry = m_index.findEntry(ref.m_uid);
        if (entry)
        {
            if (ref.m_type == AssetType::UNKNOWN)
            {
                ref.m_type = entry->type;
            }
            if (isValidAsset(entry->contentHash))
            {
                ref.m_libId = entry->contentHash;
            }
        }
    }

    if (isValidAsset(ref.m_libId))
    {
        if (auto loaded = m_cache.loadFromLibrary<T>(ref, m_importers, m_index))
        {
            return loaded;
        }
    }

#ifndef GAME_RELEASE
    const AssetIndexEntry* entry = m_index.findEntry(ref.m_uid);
    if (!entry || entry->sourcePath.empty())
    {
        return nullptr;
    }

    importAsset(entry->sourcePath, ref);
    if (!isValidAsset(ref.m_libId))
    {
        return nullptr;
    }

    return m_cache.loadFromLibrary<T>(ref, m_importers, m_index);
#else
    return nullptr;
#endif
}