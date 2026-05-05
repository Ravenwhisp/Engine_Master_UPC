#pragma once

#include <cstddef>

#include "ScriptFieldInfo.h"
#include "ScriptFieldHandlerRegistry.h"

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
    { DisplayName, ScriptFieldType::Float, offsetof(ThisScript, MemberName), getFloatFieldHandler(), { Min, Max, Speed } }

#define SERIALIZED_INT(MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Int, offsetof(ThisScript, MemberName), getIntFieldHandler() }

#define SERIALIZED_BOOL(MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Bool, offsetof(ThisScript, MemberName), getBoolFieldHandler() }

#define SERIALIZED_VEC3(MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::Vec3, offsetof(ThisScript, MemberName), getVec3FieldHandler() }

#define SERIALIZED_STRING(MemberName, DisplayName) \
    { DisplayName, ScriptFieldType::String, offsetof(ThisScript, MemberName), getStringFieldHandler() }

#define SERIALIZED_ENUM_INT(MemberName, DisplayName, NamesArray, Count) \
    { DisplayName, ScriptFieldType::EnumInt, offsetof(ThisScript, MemberName), getEnumIntFieldHandler(), {}, { NamesArray, Count } }

#define SERIALIZED_COMPONENT_REF(MemberName, DisplayName, ComponentTypeValue) \
    { DisplayName, ScriptFieldType::ComponentRef, offsetof(ThisScript, MemberName), getComponentRefFieldHandler(), {}, {}, { ComponentTypeValue } }

#define SERIALIZED_COMPONENT_REF_LIST(MemberName, DisplayName, ComponentTypeValue) \
    { DisplayName, ScriptFieldType::ComponentRefList, offsetof(ThisScript, MemberName), getComponentRefListFieldHandler(), {}, {}, { ComponentTypeValue } }