#include "Globals.h"
#include "ImporterSoundBank.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <fstream>
#include <string>

bool ImporterSoundBank::loadExternal(const std::filesystem::path& path, SoundBankSourceData& out)
{
    out.bankName = path.filename().string();

    std::filesystem::path jsonPath = path;
    jsonPath.replace_extension(".json");

    if (!std::filesystem::exists(jsonPath))
    {
        DEBUG_ERROR("[ImporterSoundBank] JSON not found: %s", jsonPath.string().c_str());
        return false;
    }

    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[ImporterSoundBank] Failed to open: %s", jsonPath.string().c_str());
        return false;
    }

    rapidjson::IStreamWrapper streamWrapper(file);
    rapidjson::Document document;
    document.ParseStream(streamWrapper);

    if (!document.IsObject() || !document.HasMember("SoundBanksInfo"))
    {
        DEBUG_ERROR("[ImporterSoundBank] Invalid JSON structure in: %s", jsonPath.string().c_str());
        return false;
    }

    const rapidjson::Value& soundBanksInfo = document["SoundBanksInfo"];
    if (!soundBanksInfo.HasMember("SoundBanks"))
    {
        DEBUG_ERROR("[ImporterSoundBank] Missing SoundBanks in: %s", jsonPath.string().c_str());
        return false;
    }

    const rapidjson::Value& soundBanks = soundBanksInfo["SoundBanks"];
    for (rapidjson::SizeType i = 0; i < soundBanks.Size(); ++i)
    {
        const rapidjson::Value& bank = soundBanks[i];
        if (!bank.HasMember("Events"))
            continue;

        const rapidjson::Value& events = bank["Events"];
        for (rapidjson::SizeType j = 0; j < events.Size(); ++j)
        {
            const rapidjson::Value& jsonEvent = events[j];
            if (!jsonEvent.HasMember("Name") || !jsonEvent.HasMember("Id"))
                continue;

            WwiseEvent event;
            event.name = jsonEvent["Name"].GetString();
            event.id = static_cast<AkUniqueID>(std::stoul(jsonEvent["Id"].GetString()));
            out.events.push_back(event);
        }
    }

    return true;
}

void ImporterSoundBank::importTyped(const SoundBankSourceData& source, SoundBankAsset* dst)
{
    dst->setBankName(source.bankName);
    for (const auto& e : source.events)
        dst->addEvent(e);
}
