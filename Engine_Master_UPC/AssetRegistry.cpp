#include "Globals.h"
#include "AssetRegistry.h"
#include "UID.h"
#include "Metadata.h"

#include <filesystem>
#include <MD5.h>

Metadata* AssetRegistry::getMetadata(const UID& uid)
{
    auto it = m_metadataMap.find(uid);
    return it != m_metadataMap.end() ? &it->second : nullptr;
}

const Metadata* AssetRegistry::getMetadata(const UID& uid) const
{
    auto it = m_metadataMap.find(uid);
    return it != m_metadataMap.end() ? &it->second : nullptr;
}

UID AssetRegistry::findByPath(const std::filesystem::path& sourcePath) const
{
    auto it = m_pathIndex.find(sourcePath.lexically_normal().string());
    return it != m_pathIndex.end() ? it->second : INVALID_UID;
}

void AssetRegistry::registerAsset(const Metadata& meta)
{
    Metadata normalised = meta;
    normalised.sourcePath = meta.sourcePath.lexically_normal();


    auto existing = m_metadataMap.find(normalised.uid);
    if (existing != m_metadataMap.end())
    {
        m_contentHashSet.erase(existing->second.contentHash);
        for (const DependencyRecord& dep : existing->second.m_dependencies)
        {
            m_contentHashSet.erase(dep.contentHash);
        }
    }

    if (isValidAsset(normalised.contentHash))
    {
        m_contentHashSet.insert(normalised.contentHash);
    }
    for (const DependencyRecord& dep : normalised.m_dependencies)
    {
        if (isValidAsset(dep.contentHash))
        {
            m_contentHashSet.insert(dep.contentHash);
        }
    }

    m_pathIndex[normalised.sourcePath.string()] = normalised.uid;
    m_metadataMap[normalised.uid] = std::move(normalised);
}

void AssetRegistry::remove(const UID& uid)
{
    auto it = m_metadataMap.find(uid);
    if (it == m_metadataMap.end())
        return;

    const Metadata& meta = it->second;

    m_contentHashSet.erase(meta.contentHash);
    for (const DependencyRecord& dep : meta.m_dependencies)
    {
        m_contentHashSet.erase(dep.contentHash);
    }

    m_pathIndex.erase(meta.sourcePath.string());
    m_metadataMap.erase(it);
}

bool AssetRegistry::contains(const UID& uid) const
{
    return m_metadataMap.count(uid) > 0;
}

bool AssetRegistry::containsContentHash(const MD5Hash& contentHash) const
{
    return m_contentHashSet.count(contentHash) > 0;
}

void AssetRegistry::clear()
{
    m_metadataMap.clear();
    m_pathIndex.clear();
    m_contentHashSet.clear();
}