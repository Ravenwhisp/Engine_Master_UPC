#include "Globals.h"
#include "ModuleMusic.h"

#include "Application.h"
#include "ModuleInput.h"

#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/Comm/AkCommunication.h>

#include <AkDefaultIOHookDeferred.h>

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

bool ModuleMusic::init()
{
	m_moduleInput = app->getModuleInput();

	DEBUG_LOG("[Module Music] Init");

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
		DEBUG_LOG("[Module Music] Wwise Communication initialized");
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

	DEBUG_LOG("[Module Music] Listener configured");

	AkBankID bankID;

	if (AK::SoundEngine::LoadBank("Init.bnk", bankID) != AK_Success)
	{
		DEBUG_ERROR("[Module Music] Failed loading Init.bnk");
		return false;
	}

	if (AK::SoundEngine::LoadBank("Main.bnk", bankID) != AK_Success)
	{
		DEBUG_ERROR("[Module Music] Failed loading Main.bnk");
		return false;
	}

	DEBUG_LOG("[Module Music] Wwise initialized successfully");

	return true;
}

void ModuleMusic::update()
{
	if (!g_soundEngineCreated)
		return;

	if (m_moduleInput && m_moduleInput->isKeyJustPressed(Keyboard::Keys::F1))
	{
		DEBUG_LOG("[Module Music] F1 -> Play_Sound02");

		AkPlayingID playingID =
			AK::SoundEngine::PostEvent("Play_Sound02", MUSIC_GAME_OBJECT);

		if (playingID == AK_INVALID_PLAYING_ID)
			DEBUG_ERROR("[Module Music] Failed posting Play_Sound02");
		else
			DEBUG_LOG("[Module Music] Play_Sound02 posted");
	}

	AK::SoundEngine::RenderAudio();
}

bool ModuleMusic::cleanUp()
{
	DEBUG_LOG("[Module Music] Cleanup");

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

	return true;
}