#pragma once

#include <string>
#include <AK/SoundEngine/Common/AkTypes.h>

struct PlayingSound
{
	std::string bankName;
	std::string eventName;
	AkPlayingID playingID = AK_INVALID_PLAYING_ID;
};