#pragma once
#include "UID.h"
#include "AssetType.h"
#include "MD5Fwd.h"
#include <filesystem>
#include <string>
#include <unordered_map>

struct AssetIndexEntry
{
    AssetType type = AssetType::UNKNOWN;
    std::filesystem::path sourcePath;
    MD5Hash contentHash = INVALID_ASSET_ID;
};

class AssetIndex
{
public:
    void registerEntry(const UID& uid, AssetType type,
                       const std::filesystem::path& sourcePath,
                       const MD5Hash& contentHash = INVALID_ASSET_ID);

    void unregister(const UID& uid);
    void unregisterByPath(const std::filesystem::path& sourcePath);

    UID findUID(const std::filesystem::path& sourcePath) const;
    const AssetIndexEntry* findEntry(const UID& uid) const;
    AssetIndexEntry* findEntryMutable(const UID& uid);

    bool contains(const UID& uid) const;
    size_t size() const;

    const std::unordered_map<UID, AssetIndexEntry>& allEntries() const { return m_uidIndex; }
    const std::unordered_map<std::string, UID>& allPaths() const { return m_pathIndex; }

private:
    std::unordered_map<UID, AssetIndexEntry>  m_uidIndex;
    std::unordered_map<std::string, UID>      m_pathIndex;
};
