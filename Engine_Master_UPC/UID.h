#pragma once
#include <cstdint>

#ifndef ENGINE_API
#ifdef ENGINE_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif
#endif

using UID = uint64_t;
constexpr UID INVALID_UID = 0;

ENGINE_API bool isValidUID(UID uid);

ENGINE_API UID GenerateUID();

