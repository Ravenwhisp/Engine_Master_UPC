#pragma once
#include "Asset.h"
#include "UID.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PrefabOverrideRecord
{
    std::unordered_map<int, std::unordered_set<std::string>> m_modifiedProperties;
    std::vector<int> m_addedComponentTypes;
    std::vector<int> m_removedComponentTypes;

    bool isEmpty() const
    {
        return m_modifiedProperties.empty()
            && m_addedComponentTypes.empty()
            && m_removedComponentTypes.empty();
    }

    void clear()
    {
        m_modifiedProperties.clear();
        m_addedComponentTypes.clear();
        m_removedComponentTypes.clear();
    }
};


struct PrefabData
{
    // ── Identity ──────────────────────────────────────────────────────────────
    std::filesystem::path m_sourcePath;      // full canonical path; used for all I/O
    std::string           m_name;            // display name = stem of m_sourcePath
    UID               m_assetUID;        // asset system key

    // ── Persistence payload ───────────────────────────────────────────────────
    std::string m_json;

};

class PrefabAsset : public Asset
{
public:
    friend class ImporterPrefab;
    friend class ImporterGltf;

    PrefabAsset() = default;
    explicit PrefabAsset(UID id) : Asset(id, AssetType::PREFAB)
    {
        m_data.m_assetUID = id;
    }

    PrefabData& getData() { return m_data; }
    const PrefabData& getData() const { return m_data; }

    // Convenience accessors.
    const std::string& getJSON()       const { return m_data.m_json; }
    const std::string& getName()       const { return m_data.m_name; }
    const std::filesystem::path& getSourcePath() const { return m_data.m_sourcePath; }
    UID                      getAssetUID()   const { return m_data.m_assetUID; }
private:
    PrefabData m_data;
};