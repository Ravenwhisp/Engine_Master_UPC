#pragma once

#include "Asset.h"
#include "IArchive.h"

#include <cstdint>
#include <vector>

class LutAsset : public Asset
{
public:
    LutAsset() { m_type = AssetType::LUT; }
    explicit LutAsset(AssetId& id) : Asset(id, AssetType::LUT) {}

    const std::vector<uint8_t>& getData() const { return m_data; }
    void setData(std::vector<uint8_t>&& data) { m_data = std::move(data); }
    bool isValid() const { return !m_data.empty(); }

    void serialize(IArchive& archive) override
    {
        uint32_t dataSize = static_cast<uint32_t>(m_data.size());
        archive.serialize(dataSize, "dataSize");
        if (archive.mode() == ArchiveMode::Input) m_data.resize(dataSize);
        archive.serializeRaw(m_data.data(), dataSize, "data");
    }

private:
    std::vector<uint8_t> m_data;
};
