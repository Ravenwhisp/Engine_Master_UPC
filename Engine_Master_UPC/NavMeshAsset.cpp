#include "NavMeshAsset.h"
#include "IArchive.h"

void NavMeshAsset::serialize(IArchive& archive)
{
    archive.serialize(m_settings.cellSize, "cellSize");
    archive.serialize(m_settings.cellHeight, "cellHeight");
    archive.serialize(m_settings.agentHeight, "agentHeight");
    archive.serialize(m_settings.agentRadius, "agentRadius");
    archive.serialize(m_settings.agentMaxClimb, "agentMaxClimb");
    archive.serialize(m_settings.agentMaxSlope, "agentMaxSlope");

    archive.serializeRaw(&m_params, sizeof(m_params), "params");

    uint32_t tileCount = static_cast<uint32_t>(m_tileRefs.size());
    archive.beginArray(tileCount, "tiles");
    m_tileRefs.resize(tileCount);
    m_tileData.resize(tileCount);
    for (uint32_t i = 0; i < tileCount; ++i)
    {
        archive.serialize(m_tileRefs[i], "tileRef");
        uint32_t dataSize = static_cast<uint32_t>(m_tileData[i].size());
        archive.serialize(dataSize, "dataSize");
        m_tileData[i].resize(dataSize);
        archive.serializeRaw(m_tileData[i].data(), dataSize, "data");
    }
}
