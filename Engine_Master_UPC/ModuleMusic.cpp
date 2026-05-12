// ModuleMusic.cpp

#include "Globals.h"
#include "ModuleMusic.h"

#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/Comm/AkCommunication.h>

#include <AkDefaultIOHookDeferred.h>

#include <filesystem>

constexpr const char* WWISE_ASSETS_PATH = "Assets\\Audio\\";

static CAkDefaultIOHookDeferred g_lowLevelIO;

static bool g_memoryCreated = false;
static bool g_streamCreated = false;
static bool g_lowLevelIOCreated = false;
static bool g_soundEngineCreated = false;
static bool g_commCreated = false;
static bool g_musicGameObjectRegistered = false;
static bool g_listenerGameObjectRegistered = false;

static constexpr AkGameObjectID MUSIC_GAME_OBJECT = 1;
static constexpr AkGameObjectID LISTENER_GAME_OBJECT = 2;

ModuleMusic::ModuleMusic() = default;
ModuleMusic::~ModuleMusic() = default;

#define region Game Loop
bool ModuleMusic::init()
{
	if (!initWwise())
	{
		return false;
	}

	if (!loadBanksFromFolder())
	{
		return false;
	}

	DEBUG_LOG("[Module Music] Initialized");

	return true;
}

void ModuleMusic::update()
{
	if (!g_soundEngineCreated)
	{
		return;
	}

	AK::SoundEngine::RenderAudio();
}

bool ModuleMusic::cleanUp()
{
	for (WwiseBank& bank : m_banks)
	{
		bank.cleanUp();
	}

	m_banks.clear();

	cleanUpWwise();

	return true;
}
#define endregion

#pragma region Wwise wrapper
bool ModuleMusic::initWwise()
{
	AkMemSettings memSettings;
	AK::MemoryMgr::GetDefaultSettings(memSettings);

	if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
	{
		DEBUG_ERROR("[Module Music] Failed initializing MemoryMgr");
		return false;
	}

	g_memoryCreated = true;

	AkStreamMgrSettings streamSettings;
	AK::StreamMgr::GetDefaultSettings(streamSettings);

	if (!AK::StreamMgr::Create(streamSettings))
	{
		DEBUG_ERROR("[Module Music] Failed creating StreamMgr");
		return false;
	}

	g_streamCreated = true;

	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

	if (g_lowLevelIO.Init(deviceSettings) != AK_Success)
	{
		DEBUG_ERROR("[Module Music] Failed initializing LowLevelIO");
		return false;
	}

	g_lowLevelIOCreated = true;

	g_lowLevelIO.SetBasePath(L"Assets\\Audio\\");

	AkInitSettings initSettings;
	AkPlatformInitSettings platformSettings;

	AK::SoundEngine::GetDefaultInitSettings(initSettings);
	AK::SoundEngine::GetDefaultPlatformInitSettings(platformSettings);

	if (AK::SoundEngine::Init(&initSettings, &platformSettings) != AK_Success)
	{
		DEBUG_ERROR("[Module Music] Failed initializing SoundEngine");
		return false;
	}

	g_soundEngineCreated = true;

	AkCommSettings commSettings;
	AK::Comm::GetDefaultInitSettings(commSettings);

	if (AK::Comm::Init(commSettings) == AK_Success)
	{
		g_commCreated = true;
	}

	if (AK::SoundEngine::RegisterGameObj(MUSIC_GAME_OBJECT, "Music") != AK_Success)
	{
		DEBUG_ERROR("[Module Music] Failed registering Music GameObject");
		return false;
	}

	g_musicGameObjectRegistered = true;

	if (AK::SoundEngine::RegisterGameObj(LISTENER_GAME_OBJECT, "Listener") != AK_Success)
	{
		DEBUG_ERROR("[Module Music] Failed registering Listener GameObject");
		return false;
	}

	g_listenerGameObjectRegistered = true;

	AK::SoundEngine::SetDefaultListeners(&LISTENER_GAME_OBJECT, 1);
	AK::SoundEngine::SetListeners(MUSIC_GAME_OBJECT, &LISTENER_GAME_OBJECT, 1);

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

void ModuleMusic::cleanUpWwise()
{
	if (g_listenerGameObjectRegistered)
	{
		AK::SoundEngine::UnregisterGameObj(LISTENER_GAME_OBJECT);
		g_listenerGameObjectRegistered = false;
	}

	if (g_musicGameObjectRegistered)
	{
		AK::SoundEngine::UnregisterGameObj(MUSIC_GAME_OBJECT);
		g_musicGameObjectRegistered = false;
	}

	if (g_commCreated)
	{
		AK::Comm::Term();
		g_commCreated = false;
	}

	if (g_soundEngineCreated)
	{
		AK::SoundEngine::Term();
		g_soundEngineCreated = false;
	}

	if (g_lowLevelIOCreated)
	{
		g_lowLevelIO.Term();
		g_lowLevelIOCreated = false;
	}

	if (g_streamCreated && AK::IAkStreamMgr::Get())
	{
		AK::IAkStreamMgr::Get()->Destroy();
		g_streamCreated = false;
	}

	if (g_memoryCreated)
	{
		AK::MemoryMgr::Term();
		g_memoryCreated = false;
	}
}
#pragma endregion

void ModuleMusic::postEvent(const char* bankName, const char* eventName)
{
	for (const WwiseBank& bank : m_banks)
	{
		if (bank.getName() == bankName)
		{
			if (bank.postEvent(eventName, MUSIC_GAME_OBJECT))
			{
				return;
			}
		}
	}

	DEBUG_ERROR("[Module Music] Event not found in any bank: %s", eventName);
}

