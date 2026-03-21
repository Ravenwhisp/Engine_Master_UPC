#pragma once
#include "MD5Fwd.h"
#include <filesystem>

struct Metadata;

// In-memory registry of all known assets.
class AssetRegistry
{
public:
    Metadata* getMetadata(const MD5Hash& uid);
    const Metadata* getMetadata(const MD5Hash& uid) const;

    MD5Hash findByPath(const std::filesystem::path& sourcePath) const;

    void registerAsset(const Metadata& meta);

    void remove(const MD5Hash& uid);

    bool contains(const MD5Hash& uid) const;

    void clear();

private:
    std::unordered_map<MD5Hash, Metadata> m_metadataMap;
    std::unordered_map<std::string, MD5Hash>   m_pathIndex;
};