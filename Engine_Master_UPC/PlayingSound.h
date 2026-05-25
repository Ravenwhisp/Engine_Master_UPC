#pragma once

#include <cstdint>
#include <string>

enum class PlayingSoundState : uint8_t
{
	Playing,
	Paused,
	Stopped
};

struct PlayingSound
{
	std::string bankName;
	std::string eventName;

	uint32_t playingID = 0;

	PlayingSoundState state = PlayingSoundState::Playing;
};