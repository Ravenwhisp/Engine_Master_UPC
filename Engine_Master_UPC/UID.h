#pragma once
#include <cstdint>

using UID = uint64_t;

inline constexpr const int INVALID_UID = 0;

bool isValidUID(const UID uid);

UID GenerateUID();

