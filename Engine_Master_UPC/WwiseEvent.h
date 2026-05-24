#pragma once
#include <string>

using AkUniqueID = unsigned int;

struct WwiseEvent
{
	std::string name;
	AkUniqueID id = 0;
};