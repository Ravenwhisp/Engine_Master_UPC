#pragma once

#include <cstddef>

#include "ScriptFieldInfo.h"

#define SCRIPT_FIELD_COUNT(FieldArrayName) \
    (sizeof(FieldArrayName) / sizeof(ScriptFieldInfo))


#define IMPLEMENT_SCRIPT_FIELDS(ScriptType, ...)                              \
    ScriptFieldList ScriptType::getExposedFields() const                      \
    {                                                                         \
        using ThisScript = ScriptType;                                        \
        static const ScriptFieldInfo ownFields[] = { __VA_ARGS__ };           \
        return ScriptFieldList(ownFields, SCRIPT_FIELD_COUNT(ownFields));     \
    }


#define IMPLEMENT_SCRIPT_FIELDS_INHERITED(ScriptType, BaseType, ...)          \
    ScriptFieldList ScriptType::getExposedFields() const                      \
    {                                                                         \
        using ThisScript = ScriptType;                                        \
        ScriptFieldList exposedFields = BaseType::getExposedFields();         \
        static const ScriptFieldInfo ownFields[] = { __VA_ARGS__ };           \
        exposedFields.append(ownFields, SCRIPT_FIELD_COUNT(ownFields));       \
        return exposedFields;                                                 \
    }


// Field helper macros

#define SERIALIZED_FLOAT(MemberName, DisplayName, Min, Max, Speed) \
    { DisplayName, ScriptFieldType::Float, offsetof(ThisScript, MemberName), { Min, Max, Speed } }

#define SERIALIZED_INT(MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Int, offsetof(ThisScript, MemberName) }

#define SERIALIZED_BOOL(MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Bool, offsetof(ThisScript, MemberName) }

#define SERIALIZED_VEC3(MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Vec3, offsetof(ThisScript, MemberName) }

#define SERIALIZED_STRING(MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::String, offsetof(ThisScript, MemberName) }

#define SERIALIZED_ENUM_INT(MemberName, DisplayName, NamesArray, Count) \
    { DisplayName, ScriptFieldType::EnumInt, offsetof(ThisScript, MemberName), {}, { NamesArray, Count } }

#define SERIALIZED_COMPONENT_REF(MemberName, DisplayName, ComponentTypeValue) \
    { DisplayName, ScriptFieldType::ComponentRef, offsetof(ThisScript, MemberName), {}, {}, { ComponentTypeValue } }