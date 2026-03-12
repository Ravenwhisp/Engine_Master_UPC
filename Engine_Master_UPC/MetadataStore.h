#pragma once
#include "Asset.h"

class MetadataStore
{
public:
    AssetMetadata* getMetadata(UID uid);
    UID            findByPath(const std::filesystem::path& sourcePath) const;
    void           registerMetadata(const AssetMetadata& meta, const std::filesystem::path& sourcePath) ;
    bool           contains(UID uid) const;
    void           clear();

private:
    std::unordered_map<UID, AssetMetadata> m_metadataMap;
    std::unordered_map<std::string, UID>   m_pathIndex;
};