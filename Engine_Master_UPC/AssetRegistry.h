#pragma once
#include <filesystem>
#include "UID.h"

class Metadata;

// In-memory registry of all known assets.
class AssetRegistry
{
public:
    Metadata* getMetadata(const UID& uid);
    const Metadata* getMetadata(const UID& uid) const;

    UID findByPath(const std::filesystem::path& sourcePath) const;

    void registerAsset(const Metadata& meta);

    void remove(const UID& uid);

    bool contains(const UID& uid) const;

    void clear();

private:
    std::unordered_map<UID, Metadata> m_metadataMap;
    std::unordered_map<std::string, UID>   m_pathIndex;
};