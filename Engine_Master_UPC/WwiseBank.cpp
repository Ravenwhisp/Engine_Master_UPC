#include "Globals.h"
#include "WwiseBank.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

constexpr bool DEBUG_WWISE_BANK = false;
#define WWISE_BANK_LOG(...) do { if constexpr (DEBUG_WWISE_BANK) { DEBUG_LOG(__VA_ARGS__); } } while (false)

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

#undef WWISE_BANK_LOG