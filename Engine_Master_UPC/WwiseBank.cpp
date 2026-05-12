#include "Globals.h"
#include "WwiseBank.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <fstream>

constexpr bool DEBUG_WWISE_BANK = false;
#define WWISE_BANK_LOG(...) do { if constexpr (DEBUG_WWISE_BANK) { DEBUG_LOG(__VA_ARGS__); } } while (false)

bool WwiseBank::init(const char* bankName, const char* jsonPath)
{
	m_bankName = bankName;
	m_jsonPath = jsonPath;

	loadEventsFromJson();

	return load();
}

void WwiseBank::cleanUp()
{
	unload();

	m_events.clear();
}

bool WwiseBank::load()
{
	if (AK::SoundEngine::LoadBank(m_bankName.c_str(), m_bankID) != AK_Success)
	{
		DEBUG_ERROR("[WwiseBank] Failed loading bank: %s", m_bankName.c_str());
		return false;
	}
	WWISE_BANK_LOG("[WwiseBank] Loaded bank: %s", m_bankName.c_str());
	return true;
}

void WwiseBank::unload()
{
	AK::SoundEngine::UnloadBank(m_bankID, nullptr);
	WWISE_BANK_LOG("[WwiseBank] Unloaded bank: %s", m_bankName.c_str());
}

bool WwiseBank::postEvent(const char* eventName, AkGameObjectID gameObjectID) const
{
	for (const WwiseEvent& event : m_events)
	{
		if (event.name != eventName)
		{
			continue;
		}

		AK::SoundEngine::PostEvent(event.id, gameObjectID);

		return true;
	}

	return false;
}

const std::vector<WwiseEvent>& WwiseBank::getEvents() const
{
	return m_events;
}

void WwiseBank::loadEventsFromJson()
{
	std::ifstream file(m_jsonPath);

	if (!file.is_open())
	{
		DEBUG_ERROR("[WwiseBank] Failed opening json: %s", m_jsonPath.c_str());
		return;
	}

	rapidjson::IStreamWrapper streamWrapper(file);

	rapidjson::Document document;
	document.ParseStream(streamWrapper);

	if (!document.IsObject())
	{
		DEBUG_ERROR("[WwiseBank] Invalid json");
		return;
	}

	if (!document.HasMember("SoundBanksInfo"))
	{
		DEBUG_ERROR("[WwiseBank] Missing SoundBanksInfo");
		return;
	}

	const rapidjson::Value& soundBanksInfo = document["SoundBanksInfo"];

	if (!soundBanksInfo.HasMember("SoundBanks"))
	{
		DEBUG_ERROR("[WwiseBank] Missing SoundBanks");
		return;
	}

	const rapidjson::Value& soundBanks = soundBanksInfo["SoundBanks"];

	for (rapidjson::SizeType i = 0; i < soundBanks.Size(); ++i)
	{
		const rapidjson::Value& bank = soundBanks[i];
		if (!bank.HasMember("Events"))
		{
			continue;
		}

		const rapidjson::Value& events = bank["Events"];
		for (rapidjson::SizeType j = 0; j < events.Size(); ++j)
		{
			const rapidjson::Value& jsonEvent = events[j];

			if (!jsonEvent.HasMember("Name"))
			{
				continue;
			}

			if (!jsonEvent.HasMember("Id"))
			{
				continue;
			}

			WwiseEvent event;
			event.name = jsonEvent["Name"].GetString();
			event.id = static_cast<AkUniqueID>(std::stoul(jsonEvent["Id"].GetString()));
			m_events.push_back(event);
			WWISE_BANK_LOG("[WwiseBank] Registered event: %s | %u", event.name.c_str(), event.id);
		}
	}
}

#undef WWISE_BANK_LOG