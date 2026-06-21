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

	m_loaded = false;
	return loadEventsFromJson();
}

void WwiseBank::cleanUp()
{
	unload();

	m_events.clear();
}

bool WwiseBank::load()
{
	if (m_loaded)
	{
		return true;
	}

	if (!m_bankData.empty())
	{
		const AKRESULT result = AK::SoundEngine::LoadBankMemoryCopy(m_bankData.data(),
			static_cast<AkUInt32>(m_bankData.size()), m_bankID);
		if (result != AK_Success)
		{
			DEBUG_ERROR("[WwiseBank] Failed loading bank '%s' from memory (error %d)", m_bankName.c_str(), static_cast<int>(result));
			return false;
		}
	}
	else
	{
		const AKRESULT result = AK::SoundEngine::LoadBank(m_bankName.c_str(), m_bankID);
		if (result != AK_Success)
		{
			DEBUG_ERROR("[WwiseBank] Failed loading bank '%s' from file (error %d)",
				m_bankName.c_str(), static_cast<int>(result));
			return false;
		}
	}

	WWISE_BANK_LOG("[WwiseBank] Loaded bank: %s", m_bankName.c_str());

	m_loaded = true;

	return true;
}

void WwiseBank::unload()
{
	if (!m_loaded)
	{
		return;
	}

	AK::SoundEngine::UnloadBank(m_bankID, nullptr);
	WWISE_BANK_LOG("[WwiseBank] Unloaded bank: %s", m_bankName.c_str());
	
	m_loaded = false;
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

bool WwiseBank::loadEventsFromJson()
{
	std::ifstream file(m_jsonPath);

	if (!file.is_open())
	{
		DEBUG_ERROR("[WwiseBank] Failed opening json: %s", m_jsonPath.c_str());
		return false;
	}

	rapidjson::IStreamWrapper streamWrapper(file);

	rapidjson::Document document;
	document.ParseStream(streamWrapper);

	if (!document.IsObject())
	{
		DEBUG_ERROR("[WwiseBank] Invalid json");
		return false;
	}

	if (!document.HasMember("SoundBanksInfo"))
	{
		DEBUG_ERROR("[WwiseBank] Missing SoundBanksInfo");
		return false;
	}

	const rapidjson::Value& soundBanksInfo = document["SoundBanksInfo"];

	if (!soundBanksInfo.HasMember("SoundBanks"))
	{
		DEBUG_ERROR("[WwiseBank] Missing SoundBanks");
		return false;
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

	return true;
}

#undef WWISE_BANK_LOG