#pragma once
#include <cstdint>

using UID = uint64_t;
constexpr UID INVALID_UID = 0;

bool isValidUID(UID uid);

UID GenerateUID();

