#pragma once

#include <vector>
#include "ComponentType.h"
#include "FieldHandler.h"

enum class FieldType
{
    Float,
    Int,
    Bool,
    Vec3,
    EnumInt,
    ComponentRef,
    ComponentRefList,
    String,
    List,
    GroupLabel,
    GroupCollapseBegin,
    GroupCollapseEnd,
    DataContainerRef,
};

struct FieldFloatInfo
{
    float min = 0.0f;
    float max = 0.0f;
    float dragSpeed = 0.1f;
};

struct FieldEnumInfo
{
    const char** names = nullptr;
    int count = 0;
};

struct FieldComponentRefInfo
{
    ComponentType componentType;
};

struct FieldListInfo
{
    FieldType elementType;
    const FieldHandler* elementHandler;
};


struct FieldInfo
{
    const char* name;
    FieldType type;
    size_t offset;

    const FieldHandler* handler = nullptr;

    FieldFloatInfo floatInfo{};
    FieldEnumInfo enumInfo{};
    FieldComponentRefInfo componentRefInfo{ ComponentType::TRANSFORM };
    FieldListInfo listInfo{ FieldType::Float, nullptr };

    bool editorOnly = false;

    bool isDataField() const
    {
        return !editorOnly;
    }
};

struct FieldList
{
    std::vector<FieldInfo> fields;

    FieldList() = default;

    FieldList(const FieldInfo* fieldArray, size_t fieldCount)
    {
        append(fieldArray, fieldCount);
    }

    void append(const FieldInfo* fieldArray, size_t fieldCount)
    {
        fields.insert(fields.end(), fieldArray, fieldArray + fieldCount);
    }
};
