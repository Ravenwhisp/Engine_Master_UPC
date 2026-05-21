#include "Globals.h"
#include "ModuleMusic.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include <filesystem>
#include <string>

constexpr bool DEBUG_MODULE_MUSIC = false;
#define WWISE_BANK_LOG(...) do { if constexpr (DEBUG_MODULE_MUSIC) { DEBUG_LOG(__VA_ARGS__); } } while (false)
#define WWISE_BANK_ERROR(...) do { if constexpr (DEBUG_MODULE_MUSIC) { DEBUG_ERROR(__VA_ARGS__); } } while (false)

constexpr const char* WWISE_ASSETS_PATH = "Assets\\Audio\\";

ModuleMusic::ModuleMusic() = default;
ModuleMusic::~ModuleMusic() = default;

#pragma region GameLoop
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

	WWISE_BANK_LOG("[Module Music] Initialized");

	return true;
}

void ModuleMusic::update()
{
	m_wwiseManager.update();
	m_playbackTracker.update();
}

bool ModuleMusic::cleanUp()
{
	m_initBnk.unload();
	m_initBnk.cleanUp();

	m_playbackTracker.cleanUp();

	for (WwiseBank& bank : m_banks)
	{
		bank.cleanUp();
	}

	m_banks.clear();

	m_wwiseManager.cleanUp();

	return true;
}
#pragma endregion

bool ModuleMusic::loadBanksFromFolder()
{
	if (!std::filesystem::exists(WWISE_ASSETS_PATH))
	{
		WWISE_BANK_ERROR("[Module Music] Audio folder not found: %s", WWISE_ASSETS_PATH);
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
			WWISE_BANK_ERROR("[Module Music] Missing bank file for json: %s", jsonPath.c_str());
			continue;
		}

		WwiseBank bank;

		if (!bank.init(bankName.c_str(), jsonPath.c_str()))
		{
			WWISE_BANK_ERROR("[Module Music] Failed loading bank: %s", bankName.c_str());
			return false;
		}

		if (bank.getName() == "Init.bnk")
		{
			m_initBnk = bank;
			m_initBnk.load();
			WWISE_BANK_LOG("Detected and loaded bank: %s", bank.getName().c_str());
		}
		else 
		{
			WWISE_BANK_LOG("Detected bank: %s", bank.getName().c_str());
			m_banks.push_back(bank);
		}

	}

	WWISE_BANK_LOG("[Module Music] Detected banks: %zu", m_banks.size());

	return true;
}

#pragma region API
void ModuleMusic::postGLobalEvent(const char* bankName, const char* eventName)
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

			const AkPlayingID playingID = AK::SoundEngine::PostEvent(event.id, m_wwiseManager.getGlobalGameObject(), AK_EndOfEvent, MusicPlaybackTracker::getCallbackFunction(), &m_playbackTracker);

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

void ModuleMusic::stopEvent(uint32_t playingID)
{
	AK::SoundEngine::StopPlayingID(static_cast<AkPlayingID>(playingID));

	m_playbackTracker.setState(playingID, PlayingSoundState::Stopped);
}

void ModuleMusic::pauseEvent(uint32_t playingID)
{
	AK::SoundEngine::ExecuteActionOnPlayingID(AK::SoundEngine::AkActionOnEventType_Pause, static_cast<AkPlayingID>(playingID));

	m_playbackTracker.setState(playingID, PlayingSoundState::Paused);
}

void ModuleMusic::resumeEvent(uint32_t playingID)
{
	AK::SoundEngine::ExecuteActionOnPlayingID(AK::SoundEngine::AkActionOnEventType_Resume,static_cast<AkPlayingID>(playingID));

	m_playbackTracker.setState(playingID, PlayingSoundState::Playing);
}
#pragma endregion


#pragma region Extra
void ModuleMusic::unloadAllBanks()
{
	for (WwiseBank& bank : m_banks)
	{
		if (bank.isLoaded())
		{
			bank.unload();
		}
	}

	WWISE_BANK_LOG("[Module Music] All banks unloaded.");
}

bool ModuleMusic::loadBank(const std::string& bankName)
{
	for (WwiseBank& bank : m_banks)
	{
		if (bank.getName() != bankName)
		{
			continue;
		}

		return bank.load();
	}

	WWISE_BANK_ERROR("[Module Music] Bank not found: %s", bankName.c_str());
	return false;
}

bool ModuleMusic::unloadBank(const std::string& bankName)
{
	for (WwiseBank& bank : m_banks)
	{
		if (bank.getName() != bankName)
		{
			continue;
		}

		bank.unload();
		return true;
	}

	WWISE_BANK_ERROR("[Module Music] Bank not found: %s", bankName.c_str());
	return false;
}
#pragma endregion

#undef WWISE_BANK_LOG
#undef WWISE_BANK_ERROR