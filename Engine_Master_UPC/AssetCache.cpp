#include "Globals.h"
#include "AssetCache.h"
#include "Asset.h"

bool AssetCache::isLoaded(const UID& uid)
{
    return m_cache.contains(uid);
}

void AssetCache::unload(const UID& uid)
{
    m_cache.remove(uid);
}

void AssetCache::clear()
{
    m_cache.clear();
}

std::shared_ptr<Asset> AssetCache::get(const UID& uid)
{
    return m_cache.get(uid);
}

void AssetCache::insert(const UID& uid, std::shared_ptr<Asset> asset)
{
    m_cache.insert(uid, asset);
}
