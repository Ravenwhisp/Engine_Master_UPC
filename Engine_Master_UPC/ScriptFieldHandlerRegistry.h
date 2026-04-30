#pragma once

#include "EngineAPI.h"


struct ScriptFieldHandler;

ENGINE_API const ScriptFieldHandler* getFloatFieldHandler();
ENGINE_API const ScriptFieldHandler* getComponentRefFieldHandler();