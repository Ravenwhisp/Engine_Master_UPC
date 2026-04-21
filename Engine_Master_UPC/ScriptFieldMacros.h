#pragma once

#include <cstddef>

#include "ScriptFieldInfo.h"

#define SCRIPT_FIELD_COUNT(FieldArrayName) \
    (sizeof(FieldArrayName) / sizeof(ScriptFieldInfo))


#define IMPLEMENT_SCRIPT_FIELDS(ScriptType, ...)                              \
    ScriptFieldList ScriptType::getExposedFields() const                      \
    {                                                                         \
        static const ScriptFieldInfo ownFields[] = { __VA_ARGS__ };           \
        return ScriptFieldList(ownFields, SCRIPT_FIELD_COUNT(ownFields));     \
    }


#define IMPLEMENT_SCRIPT_FIELDS_INHERITED(ScriptType, BaseType, ...)          \
    ScriptFieldList ScriptType::getExposedFields() const                      \
    {                                                                         \
        ScriptFieldList exposedFields = BaseType::getExposedFields();         \
        static const ScriptFieldInfo ownFields[] = { __VA_ARGS__ };           \
        exposedFields.append(ownFields, SCRIPT_FIELD_COUNT(ownFields));       \
        return exposedFields;                                                 \
    }


// Field helper macros

#define SERIALIZED_FLOAT(ScriptType, MemberName, DisplayName, Min, Max, Speed) \
    { DisplayName, ScriptFieldType::Float, offsetof(ScriptType, MemberName), { Min, Max, Speed } }

#define SERIALIZED_INT(ScriptType, MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Int, offsetof(ScriptType, MemberName) }

#define SERIALIZED_BOOL(ScriptType, MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Bool, offsetof(ScriptType, MemberName) }

#define SERIALIZED_VEC3(ScriptType, MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Vec3, offsetof(ScriptType, MemberName) }

#define SERIALIZED_STRING(ScriptType, MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::String, offsetof(ScriptType, MemberName) }

#define SERIALIZED_ENUM_INT(ScriptType, MemberName, DisplayName, NamesArray, Count) \
    { DisplayName, ScriptFieldType::EnumInt, offsetof(ScriptType, MemberName), {}, { NamesArray, Count } }

#define SERIALIZED_COMPONENT_REF(ScriptType, MemberName, DisplayName, ComponentTypeValue) \
    { DisplayName, ScriptFieldType::ComponentRef, offsetof(ScriptType, MemberName), {}, {}, { ComponentTypeValue } }