#pragma once
#include "Asset.h"
#include "WwiseEvent.h"
#include <vector>
#include <string>

class SoundBankAsset : public Asset
{
public:
    SoundBankAsset() { m_type = AssetType::SOUND_BANK; }
    SoundBankAsset(AssetReference& id) : Asset(id, AssetType::SOUND_BANK) {}

    const std::string& getBankName() const { return m_bankName; }
    void setBankName(const std::string& name) { m_bankName = name; }

    const std::vector<WwiseEvent>& getEvents() const { return m_events; }
    void addEvent(const WwiseEvent& e) { m_events.push_back(e); }
    void clearEvents() { m_events.clear(); }

    bool isValid() const { return !m_bankName.empty(); }

    void serialize(IArchive& archive) override;

private:
    std::string m_bankName;
    std::vector<WwiseEvent> m_events;
};
