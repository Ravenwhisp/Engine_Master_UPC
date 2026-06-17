#include "Globals.h"
#include "AssetIndex.h"
#include "UID.h"

void AssetIndex::registerEntry(const UID& uid, AssetType type,
                                const std::filesystem::path& sourcePath,
                                const MD5Hash& contentHash)
{
    const std::filesystem::path norm = sourcePath.lexically_normal();
    m_uidIndex[uid] = { type, norm, contentHash };
    if (!norm.empty())
    {
        m_pathIndex[norm.string()] = uid;
    }
}

void AssetIndex::unregister(const UID& uid)
{
    auto it = m_uidIndex.find(uid);
    if (it != m_uidIndex.end())
    {
        if (!it->second.sourcePath.empty())
        {
            m_pathIndex.erase(it->second.sourcePath.lexically_normal().string());
        }
        m_uidIndex.erase(it);
    }
}

void AssetIndex::unregisterByPath(const std::filesystem::path& sourcePath)
{
    const std::filesystem::path normPath = sourcePath.lexically_normal();
    auto pathIt = m_pathIndex.find(normPath.string());
    if (pathIt != m_pathIndex.end())
    {
        m_uidIndex.erase(pathIt->second);
        m_pathIndex.erase(pathIt);
    }
}

UID AssetIndex::findUID(const std::filesystem::path& sourcePath) const
{
    const auto it = m_pathIndex.find(sourcePath.lexically_normal().string());
    return it != m_pathIndex.end() ? it->second : INVALID_UID;
}

const AssetIndexEntry* AssetIndex::findEntry(const UID& uid) const
{
    const auto it = m_uidIndex.find(uid);
    return it != m_uidIndex.end() ? &it->second : nullptr;
}

AssetIndexEntry* AssetIndex::findEntryMutable(const UID& uid)
{
    auto it = m_uidIndex.find(uid);
    return it != m_uidIndex.end() ? &it->second : nullptr;
}

bool AssetIndex::contains(const UID& uid) const
{
    return m_uidIndex.find(uid) != m_uidIndex.end();
}

size_t AssetIndex::size() const
{
    return m_uidIndex.size();
}
