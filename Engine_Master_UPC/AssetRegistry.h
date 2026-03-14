#pragma once
#include "Asset.h"
#include <filesystem>
#include <unordered_map>

// In-memory registry of all known assets.
class AssetRegistry
{
public:
    AssetMetadata* getMetadata(UID uid);
    const AssetMetadata* getMetadata(UID uid) const;

    UID findByPath(const std::filesystem::path& sourcePath) const;

    void registerAsset(const AssetMetadata& meta);


    void remove(UID uid);

    bool contains(UID uid) const;

    void clear();

private:
    std::unordered_map<UID, AssetMetadata> m_metadataMap;
    std::unordered_map<std::string, UID>   m_pathIndex;
};