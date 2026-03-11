#pragma once

#include "GameScriptsAPI.h"
#include "ScriptFactory.h"

extern "C" GAMESCRIPTS_API const char* GetScriptName();
extern "C" GAMESCRIPTS_API ScriptCreator GetScriptCreator();