#include "Globals.h"
#include "AssetRegistry.h"
#include "UID.h"
#include "Metadata.h"

#include <filesystem>

Metadata* AssetRegistry::getMetadata(const MD5Hash& uid)
{
    auto it = m_metadataMap.find(uid);
    return it != m_metadataMap.end() ? &it->second : nullptr;
}

const Metadata* AssetRegistry::getMetadata(const MD5Hash& uid) const
{
    auto it = m_metadataMap.find(uid);
    return it != m_metadataMap.end() ? &it->second : nullptr;
}

MD5Hash AssetRegistry::findByPath(const std::filesystem::path& sourcePath) const
{
    auto it = m_pathIndex.find(sourcePath.lexically_normal().string());
    return it != m_pathIndex.end() ? it->second : INVALID_ASSET_ID;
}

void AssetRegistry::registerAsset(const Metadata& meta)
{
    // Normalise before inserting so findByPath always matches.
    Metadata normalised = meta;
    normalised.sourcePath    = meta.sourcePath.lexically_normal();

    m_pathIndex[normalised.sourcePath.string()] = normalised.uid;
    m_metadataMap[normalised.uid]               = std::move(normalised);
}

void AssetRegistry::remove(const MD5Hash& uid)
{
    auto it = m_metadataMap.find(uid);
    if (it == m_metadataMap.end())
        return;

    // sourcePath is stored on the metadata itself — direct O(1) erase.
    m_pathIndex.erase(it->second.sourcePath.string());
    m_metadataMap.erase(it);
}

bool AssetRegistry::contains(const MD5Hash& uid) const
{
    return m_metadataMap.count(uid) > 0;
}

void AssetRegistry::clear()
{
    m_metadataMap.clear();
    m_pathIndex.clear();
}