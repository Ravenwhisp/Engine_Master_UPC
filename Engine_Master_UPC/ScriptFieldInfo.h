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

    // These are not necessary at all, if we find them usseles or anoyiing to set we can just delete them
    // We can also create a struct of ScriptFieldUiInfo for example so we keep things separate
    float minFloat = 0.0f;
    float maxFloat = 0.0f;
    float dragSpeed = 0.1f;

    const char** enumNames = nullptr;
    int enumCount = 0;
};