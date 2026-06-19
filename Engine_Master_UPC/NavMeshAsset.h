#pragma once
#include "Asset.h"
#include "NavMeshResource.h"
#include <DetourNavMesh.h>
#include <vector>
#include <cstdint>

class NavMeshAsset : public Asset
{
public:
    NavMeshAsset() = default;
    NavMeshAsset(AssetReference& id) : Asset(id, AssetType::NAVMESH) {}

    const NavMeshSettings& getSettings() const { return m_settings; }
    void setSettings(const NavMeshSettings& s) { m_settings = s; }

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
        m_settings = NavMeshSettings{};
        m_params = dtNavMeshParams{};
    }

    bool isValid() const { return !m_tileRefs.empty(); }
    bool empty() const { return m_tileRefs.empty(); }

    void serialize(IArchive& archive) override;

private:
    NavMeshSettings m_settings;
    dtNavMeshParams m_params{};
    std::vector<dtTileRef> m_tileRefs;
    std::vector<std::vector<uint8_t>> m_tileData;
};
