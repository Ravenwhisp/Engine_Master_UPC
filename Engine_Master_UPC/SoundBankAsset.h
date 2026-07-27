#pragma once
#include "Asset.h"
#include "WwiseEvent.h"
#include <vector>
#include <string>
#include <cstdint>

class SoundBankAsset : public Asset
{
public:
    SoundBankAsset() { m_type = AssetType::SOUND_BANK; }
    SoundBankAsset(AssetId& id) : Asset(id, AssetType::SOUND_BANK) {}

    const std::string& getBankName() const { return m_bankName; }
    void setBankName(const std::string& name) { m_bankName = name; }

    const std::vector<WwiseEvent>& getEvents() const { return m_events; }
    void addEvent(const WwiseEvent& e) { m_events.push_back(e); }
    void clearEvents() { m_events.clear(); }

    const std::vector<uint8_t>& getBankData() const { return m_bankData; }
    void setBankData(const std::vector<uint8_t>& d) { m_bankData = d; }

    bool isValid() const { return !m_bankName.empty(); }

    void serialize(IArchive& archive) override;

private:
    std::string m_bankName;
    std::vector<WwiseEvent> m_events;
    std::vector<uint8_t> m_bankData;
};
