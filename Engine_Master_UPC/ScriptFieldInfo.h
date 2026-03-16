#pragma once

#include <cstddef>

enum class ScriptFieldType
{
    Float,
    Int,
    Bool,
    Vec3,
    EnumInt
};

struct ScriptFieldInfo
{
    const char* name;
    ScriptFieldType type;
    size_t offset;

    float minFloat = 0.0f;
    float maxFloat = 0.0f;
    float dragSpeed = 0.1f;

    const char** enumNames = nullptr;
    int enumCount = 0;
};