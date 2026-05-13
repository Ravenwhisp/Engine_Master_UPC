#include "Globals.h"
#include "ModuleMusic.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include <filesystem>
#include <string>

constexpr const char* WWISE_ASSETS_PATH = "Assets\\Audio\\";

ModuleMusic::ModuleMusic() = default;
ModuleMusic::~ModuleMusic() = default;

bool ModuleMusic::init()
{
	if (!m_wwiseManager.init())
	{
		return false;
	}

	if (!loadBanksFromFolder())
	{
		m_wwiseManager.cleanUp();
		return false;
	}

	DEBUG_LOG("[Module Music] Initialized");

	return true;
}

void ModuleMusic::update()
{
	m_wwiseManager.update();
	m_playbackTracker.update();
}

bool ModuleMusic::cleanUp()
{
	m_playbackTracker.cleanUp();

	for (WwiseBank& bank : m_banks)
	{
		bank.cleanUp();
	}

	m_banks.clear();

	m_wwiseManager.cleanUp();

	return true;
}

bool ModuleMusic::loadBanksFromFolder()
{
	if (!std::filesystem::exists(WWISE_ASSETS_PATH))
	{
		DEBUG_ERROR("[Module Music] Audio folder not found: %s", WWISE_ASSETS_PATH);
		return false;
	}

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(WWISE_ASSETS_PATH))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		if (entry.path().extension() != ".json")
		{
			continue;
		}

		const std::string stem = entry.path().stem().string();

		if (stem == "PlatformInfo" || stem == "PluginInfo")
		{
			continue;
		}

		const std::string bankName = stem + ".bnk";
		const std::string jsonPath = entry.path().string();
		const std::string bankPath = std::string(WWISE_ASSETS_PATH) + bankName;

		if (!std::filesystem::exists(bankPath))
		{
			DEBUG_ERROR("[Module Music] Missing bank file for json: %s", jsonPath.c_str());
			continue;
		}

		WwiseBank bank;

		if (!bank.init(bankName.c_str(), jsonPath.c_str()))
		{
			DEBUG_ERROR("[Module Music] Failed loading bank: %s", bankName.c_str());
			return false;
		}

		m_banks.push_back(bank);
	}

	DEBUG_LOG("[Module Music] Loaded banks: %zu", m_banks.size());

	return true;
}

void ModuleMusic::postEvent(const char* bankName, const char* eventName)
{
	for (const WwiseBank& bank : m_banks)
	{
		if (bank.getName() != bankName)
		{
			continue;
		}

		for (const WwiseEvent& event : bank.getEvents())
		{
			if (event.name != eventName)
			{
				continue;
			}

			const AkPlayingID playingID = AK::SoundEngine::PostEvent(event.id, m_wwiseManager.getMusicGameObject(), AK_EndOfEvent, MusicPlaybackTracker::getCallbackFunction(), &m_playbackTracker);

			if (playingID == AK_INVALID_PLAYING_ID)
			{
				DEBUG_ERROR("[Module Music] Failed posting event: %s", eventName);
				return;
			}

			PlayingSound playingSound;
			playingSound.bankName = bankName;
			playingSound.eventName = eventName;
			playingSound.playingID = static_cast<uint32_t>(playingID);

			m_playbackTracker.queuePlayingSoundToAdd(playingSound);

			return;
		}
	}

	DEBUG_ERROR("[Module Music] Event not found in any bank: %s", eventName);
}
