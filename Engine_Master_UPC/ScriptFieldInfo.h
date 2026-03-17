#pragma once

#include <cstddef>
#include "UID.h"
#include "ComponentType.h"

enum class ScriptFieldType
{
    Float,
    Int,
    Bool,
    Vec3,
    EnumInt,
    ComponentRef
};

struct ScriptFieldFloatInfo
{
    float min = 0.0f;
    float max = 0.0f;
    float dragSpeed = 0.1f;
};

struct ScriptFieldEnumInfo
{
    const char** names = nullptr;
    int count = 0;
};

struct ScriptFieldComponentRefInfo
{
    ComponentType componentType;
};

struct ScriptFieldInfo
{
    const char* name;
    ScriptFieldType type;
    size_t offset;

    ScriptFieldFloatInfo floatInfo{};
    ScriptFieldEnumInfo enumInfo{};
    ScriptFieldComponentRefInfo componentRefInfo{ ComponentType::TRANSFORM };
};

struct ScriptFieldList
{
    const ScriptFieldInfo* fields = nullptr;
    size_t count = 0;
};