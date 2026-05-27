#pragma once
#include "Asset.h"
#include "IArchive.h"

#include <filesystem>
#include <string>

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
    explicit PrefabAsset(AssetReference& id) : Asset(id, AssetType::PREFAB)
    {
        m_data.m_assetUID = id.m_uid;
    }

    PrefabData& getData() { return m_data; }
    const PrefabData& getData() const { return m_data; }

    // Convenience accessors.
    const std::string& getJSON()       const { return m_data.m_json; }
    const std::string& getName()       const { return m_data.m_name; }
    const std::filesystem::path& getSourcePath() const { return m_data.m_sourcePath; }
    UID                      getAssetUID()   const { return m_data.m_assetUID; }

    void serialize(IArchive& archive) override
    {
        std::string pathStr = m_data.m_sourcePath.string();
        archive.serialize(pathStr);
        if (archive.mode() == ArchiveMode::Input)
            m_data.m_sourcePath = pathStr;

        archive.serialize(m_data.m_name);
        archive.serialize(m_data.m_assetUID);
        archive.serialize(m_data.m_json);
    }

private:
    PrefabData m_data;
};