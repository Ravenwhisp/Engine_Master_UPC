#include "Globals.h"
#include "MetadataStore.h"
#include "Asset.h"
#include "UID.h"
#include <filesystem>


AssetMetadata* MetadataStore::getMetadata(UID uid)
{
    auto it = m_metadataMap.find(uid);
    if (it != m_metadataMap.end())
    {
        return &it->second;
    }
    return nullptr;
}

UID MetadataStore::findByPath(const std::filesystem::path& sourcePath) const
{
    auto it = m_pathIndex.find(sourcePath.lexically_normal().string());
    if (it != m_pathIndex.end())
    {
        return it->second;
    }
    return INVALID_ASSET_ID;
}

void MetadataStore::registerMetadata(const AssetMetadata& meta, const std::filesystem::path& sourcePath)
{
    m_metadataMap[meta.uid] = meta;
    m_pathIndex[sourcePath.lexically_normal().string()] = meta.uid;
}

bool MetadataStore::contains(UID uid) const
{
    return m_metadataMap.count(uid) > 0;
}

void MetadataStore::clear()
{
    m_metadataMap.clear();
    m_pathIndex.clear();
}