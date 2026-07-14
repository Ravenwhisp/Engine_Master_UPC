#pragma once

#include "FieldInfo.h"
#include "EngineAPI.h"

struct FieldHandler;

ENGINE_API const FieldHandler* getFloatFieldHandler();
ENGINE_API const FieldHandler* getIntFieldHandler();
ENGINE_API const FieldHandler* getBoolFieldHandler();
ENGINE_API const FieldHandler* getVec3FieldHandler();
ENGINE_API const FieldHandler* getEnumIntFieldHandler();
ENGINE_API const FieldHandler* getComponentRefFieldHandler();
ENGINE_API const FieldHandler* getComponentRefListFieldHandler();
ENGINE_API const FieldHandler* getStringFieldHandler();
ENGINE_API const FieldHandler* getListFieldHandler(FieldType elementType);

ENGINE_API const FieldHandler* getGroupLabelFieldHandler();
ENGINE_API const FieldHandler* getAssetRefFieldHandler();
