#pragma once

#include <cstdint>
#include <string>

struct PlayingSound
{
	std::string bankName;
	std::string eventName;
	uint32_t playingID = 0;
};