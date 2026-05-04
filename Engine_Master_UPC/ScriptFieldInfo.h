#pragma once

#include <vector>
#include "ComponentType.h"
#include "ScriptFieldHandler.h"

enum class ScriptFieldType
{
    Float,
    Int,
    Bool,
    Vec3,
    EnumInt,
    ComponentRef,
    ComponentRefList,
    String
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

    const ScriptFieldHandler* handler = nullptr;

    ScriptFieldFloatInfo floatInfo{};
    ScriptFieldEnumInfo enumInfo{};
    ScriptFieldComponentRefInfo componentRefInfo{ ComponentType::TRANSFORM };
};

struct ScriptFieldList
{
    std::vector<ScriptFieldInfo> fields;

    ScriptFieldList() = default;

    ScriptFieldList(const ScriptFieldInfo* fieldArray, size_t fieldCount)
    {
        append(fieldArray, fieldCount);
    }

    void append(const ScriptFieldInfo* fieldArray, size_t fieldCount)
    {
        fields.insert(fields.end(), fieldArray, fieldArray + fieldCount);
    }
};