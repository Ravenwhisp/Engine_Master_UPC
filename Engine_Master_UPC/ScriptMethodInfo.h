#pragma once

#include "AssetId.h"

class Script;

enum class ScriptMethodParamType
{
    None,
    Float,
    Int,
    Bool,
    Vec3,
    String,
    Unsupported,
    AssetId
};

using ScriptMethodParamFunc = void(*)(Script*, const void*);

struct ScriptMethodInfo
{
    const char* name;
    void(*func)(Script*);
    ScriptMethodParamType paramType = ScriptMethodParamType::None;
    const char* paramName = nullptr;
    ScriptMethodParamFunc paramFunc = nullptr;
};
