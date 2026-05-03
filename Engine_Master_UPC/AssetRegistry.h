#pragma once
#include "UID.h"
#include "MD5Fwd.h"
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

struct Metadata;

class AssetRegistry
{
public:
    Metadata* getMetadata(const UID& uid);
    const Metadata* getMetadata(const UID& uid) const;

    UID findByPath(const std::filesystem::path& sourcePath) const;

    void registerAsset(const Metadata& meta);
    void remove(const UID& uid);

    bool contains(const UID& uid) const;

    bool containsContentHash(const MD5Hash& contentHash) const;

    void clear();

private:
    std::unordered_map<UID, Metadata>       m_metadataMap;

    std::unordered_map<std::string, UID>    m_pathIndex;

    std::unordered_set<MD5Hash>             m_contentHashSet;
};