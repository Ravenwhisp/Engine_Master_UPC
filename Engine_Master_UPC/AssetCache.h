#pragma once
#include "AssetReference.h"
#include "WeakCache.h"
#include "MD5Fwd.h"
#include "UID.h"
#include <filesystem>
#include <memory>

class Asset;
class AssetIndex;
class ImporterRegistry;

class AssetCache
{
public:
    template<typename T>
    std::shared_ptr<T> load(AssetReference& ref, AssetIndex& index,
                             ImporterRegistry& importers);

    template<typename T>
    std::shared_ptr<T> loadAtPath(const std::filesystem::path& sourcePath,
                                   AssetIndex& index, ImporterRegistry& importers);

    template<typename T>
    std::shared_ptr<T> loadFromLibrary(AssetReference& ref, ImporterRegistry& importers,
                                        AssetIndex& index);

    bool isLoaded(const UID& uid);
    void unload(const UID& uid);
    void clear();

    std::shared_ptr<Asset> get(const UID& uid);
    void insert(const UID& uid, std::shared_ptr<Asset> asset);

private:

    WeakCache<UID, Asset> m_cache;
};

#include "AssetCache.inl"
