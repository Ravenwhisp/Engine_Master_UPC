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