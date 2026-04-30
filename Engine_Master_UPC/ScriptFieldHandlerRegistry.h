#pragma once

#include "EngineAPI.h"


struct ScriptFieldHandler;

ENGINE_API const ScriptFieldHandler* getFloatFieldHandler();
ENGINE_API const ScriptFieldHandler* getIntFieldHandler();
ENGINE_API const ScriptFieldHandler* getBoolFieldHandler();
ENGINE_API const ScriptFieldHandler* getVec3FieldHandler();
ENGINE_API const ScriptFieldHandler* getEnumIntFieldHandler();
ENGINE_API const ScriptFieldHandler* getComponentRefFieldHandler();
ENGINE_API const ScriptFieldHandler* getComponentRefListFieldHandler();
ENGINE_API const ScriptFieldHandler* getStringFieldHandler();