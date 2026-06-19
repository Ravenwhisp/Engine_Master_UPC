#pragma once
#include "Asset.h"
#include "NavMeshBuildSettings.h"
#include <DetourNavMesh.h>
#include <vector>
#include <cstdint>

class NavMeshAsset : public Asset
{
public:
    NavMeshAsset() = default;
    NavMeshAsset(AssetReference& id) : Asset(id, AssetType::NAVMESH) {}

    NavMeshBuildSettings& getSettings()
    {
        auto* s = static_cast<NavMeshBuildSettings*>(getImportSettings());
        if (!s)
        {
            setImportSettings(createDefaultImportSettings());
            s = static_cast<NavMeshBuildSettings*>(getImportSettings());
        }
        return *s;
    }

    const NavMeshBuildSettings& getSettings() const
    {
        return *static_cast<const NavMeshBuildSettings*>(getImportSettings());
    }

    void setSettings(const NavMeshBuildSettings& s)
    {
        auto copy = std::make_unique<NavMeshBuildSettings>(s);
        setImportSettings(std::move(copy));
    }

    const dtNavMeshParams& getParams() const { return m_params; }
    void setParams(const dtNavMeshParams& p) { m_params = p; }

    const std::vector<dtTileRef>& getTileRefs() const { return m_tileRefs; }
    const std::vector<std::vector<uint8_t>>& getTileData() const { return m_tileData; }

    void addTile(dtTileRef ref, const std::vector<uint8_t>& data)
    {
        m_tileRefs.push_back(ref);
        m_tileData.push_back(data);
    }

    void clear()
    {
        m_tileRefs.clear();
        m_tileData.clear();
        setImportSettings(createDefaultImportSettings());
        m_params = dtNavMeshParams{};
    }

    bool isValid() const { return !m_tileRefs.empty(); }
    bool empty() const { return m_tileRefs.empty(); }

    void serialize(IArchive& archive) override;

    std::unique_ptr<ImportSettings> createDefaultImportSettings() const override
    {
        return std::make_unique<NavMeshBuildSettings>();
    }

private:
    dtNavMeshParams m_params{};
    std::vector<dtTileRef> m_tileRefs;
    std::vector<std::vector<uint8_t>> m_tileData;
};
