#pragma once

#ifdef GAMESCRIPTS_EXPORTS
#define GAMESCRIPTS_API __declspec(dllexport)
#else
#define GAMESCRIPTS_API __declspec(dllimport)
#endif

extern "C" GAMESCRIPTS_API void PrintGameScriptsLoaded();