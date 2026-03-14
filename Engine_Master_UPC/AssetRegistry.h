#pragma once
#include "Asset.h"
#include <filesystem>
#include <unordered_map>

// In-memory registry of all known assets.
class AssetRegistry
{
public:
    AssetMetadata* getMetadata(const MD5Hash& uid);
    const AssetMetadata* getMetadata(const MD5Hash& uid) const;

    MD5Hash findByPath(const std::filesystem::path& sourcePath) const;

    void registerAsset(const AssetMetadata& meta);

    void remove(const MD5Hash& uid);

    bool contains(const MD5Hash& uid) const;

    void clear();

private:
    std::unordered_map<MD5Hash, AssetMetadata> m_metadataMap;
    std::unordered_map<std::string, MD5Hash>   m_pathIndex;
};