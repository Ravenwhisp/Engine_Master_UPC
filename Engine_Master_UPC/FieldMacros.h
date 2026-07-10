#pragma once

#include <cstddef>

#include "FieldInfo.h"
#include "FieldHandlerRegistry.h"
#include "FieldTypeTraits.h"

#define FIELD_COUNT(FieldArrayName) \
    (sizeof(FieldArrayName) / sizeof(FieldInfo))


#define IMPLEMENT_FIELDS(TypeName, ...)                                       \
    FieldList TypeName::getExposedFields() const                              \
    {                                                                         \
        using ThisType = TypeName;                                            \
        static const FieldInfo ownFields[] = { __VA_ARGS__ };                \
        return FieldList(ownFields, FIELD_COUNT(ownFields));                  \
    }


#define IMPLEMENT_FIELDS_INHERITED(TypeName, BaseType, ...)                   \
    FieldList TypeName::getExposedFields() const                              \
    {                                                                         \
        using ThisType = TypeName;                                            \
        FieldList exposedFields = BaseType::getExposedFields();               \
        static const FieldInfo ownFields[] = { __VA_ARGS__ };                \
        exposedFields.append(ownFields, FIELD_COUNT(ownFields));              \
        return exposedFields;                                                 \
    }

// Inline version — for placing inside the class body (header-only types like DataContainer)
#define IMPLEMENT_FIELDS_INLINE(TypeName, ...)                                \
    FieldList getExposedFields() const override                               \
    {                                                                         \
        using ThisType = TypeName;                                            \
        static const FieldInfo ownFields[] = { __VA_ARGS__ };                \
        return FieldList(ownFields, FIELD_COUNT(ownFields));                  \
    }

// Backward compatibility aliases
#define IMPLEMENT_SCRIPT_FIELDS IMPLEMENT_FIELDS
#define IMPLEMENT_SCRIPT_FIELDS_INHERITED IMPLEMENT_FIELDS_INHERITED
#define IMPLEMENT_DATACONTAINER_FIELDS IMPLEMENT_FIELDS_INLINE
#define SCRIPT_FIELD_COUNT FIELD_COUNT

// Field helper macros

#define SERIALIZED_FLOAT(MemberName, DisplayName, Min, Max, Speed) \
    { DisplayName, FieldType::Float, offsetof(ThisType, MemberName), getFloatFieldHandler(), { Min, Max, Speed } }

#define SERIALIZED_INT(MemberName, DisplayName) \
    { DisplayName, FieldType::Int, offsetof(ThisType, MemberName), getIntFieldHandler() }

#define SERIALIZED_BOOL(MemberName, DisplayName) \
    { DisplayName, FieldType::Bool, offsetof(ThisType, MemberName), getBoolFieldHandler() }

#define SERIALIZED_VEC3(MemberName, DisplayName) \
    { DisplayName, FieldType::Vec3, offsetof(ThisType, MemberName), getVec3FieldHandler() }

#define SERIALIZED_STRING(MemberName, DisplayName) \
    { DisplayName, FieldType::String, offsetof(ThisType, MemberName), getStringFieldHandler() }

#define SERIALIZED_ENUM_INT(MemberName, DisplayName, NamesArray, Count) \
    { DisplayName, FieldType::EnumInt, offsetof(ThisType, MemberName), getEnumIntFieldHandler(), {}, { NamesArray, Count } }

#define SERIALIZED_COMPONENT_REF(MemberName, DisplayName, ComponentTypeValue) \
    { DisplayName, FieldType::ComponentRef, offsetof(ThisType, MemberName), getComponentRefFieldHandler(), {}, {}, { ComponentTypeValue } }

#define SERIALIZED_ASSET_REF(MemberName, DisplayName, AssetTypeValue) \
    { DisplayName, FieldType::AssetRef, offsetof(ThisType, MemberName), getAssetRefFieldHandler() }

#define SERIALIZED_COMPONENT_REF_LIST(MemberName, DisplayName, ComponentTypeValue) \
    { DisplayName, FieldType::ComponentRefList, offsetof(ThisType, MemberName), getComponentRefListFieldHandler(), {}, {}, { ComponentTypeValue } }

// Vector (std::vector<T>) field macros

// Generic -- auto-detects the element type at compile time
#define SERIALIZED_VECTOR(MemberName, DisplayName) \
    { DisplayName, FieldType::List, offsetof(ThisType, MemberName), \
      getListFieldHandler(fieldTypeOf<typename decltype(ThisType::MemberName)::value_type>()), \
      {}, {}, {}, \
      { fieldTypeOf<typename decltype(ThisType::MemberName)::value_type>(), \
        fieldHandlerOf<typename decltype(ThisType::MemberName)::value_type>() } }

// Type-specific convenience macros (no auto-detection needed)

#define SERIALIZED_FLOAT_VECTOR(MemberName, DisplayName) \
    { DisplayName, FieldType::List, offsetof(ThisType, MemberName), getListFieldHandler(FieldType::Float), {}, {}, {}, { FieldType::Float, getFloatFieldHandler() } }

#define SERIALIZED_INT_VECTOR(MemberName, DisplayName) \
    { DisplayName, FieldType::List, offsetof(ThisType, MemberName), getListFieldHandler(FieldType::Int), {}, {}, {}, { FieldType::Int, getIntFieldHandler() } }

#define SERIALIZED_BOOL_VECTOR(MemberName, DisplayName) \
    { DisplayName, FieldType::List, offsetof(ThisType, MemberName), getListFieldHandler(FieldType::Bool), {}, {}, {}, { FieldType::Bool, getBoolFieldHandler() } }

#define SERIALIZED_VEC3_VECTOR(MemberName, DisplayName) \
    { DisplayName, FieldType::List, offsetof(ThisType, MemberName), getListFieldHandler(FieldType::Vec3), {}, {}, {}, { FieldType::Vec3, getVec3FieldHandler() } }

#define SERIALIZED_STRING_VECTOR(MemberName, DisplayName) \
    { DisplayName, FieldType::List, offsetof(ThisType, MemberName), getListFieldHandler(FieldType::String), {}, {}, {}, { FieldType::String, getStringFieldHandler() } }

#define SERIALIZED_ENUM_INT_VECTOR(MemberName, DisplayName, NamesArray, Count) \
    { DisplayName, FieldType::List, offsetof(ThisType, MemberName), getListFieldHandler(FieldType::EnumInt), {}, { NamesArray, Count }, {}, { FieldType::EnumInt, getEnumIntFieldHandler() } }

#define SERIALIZED_COMPONENT_REF_VECTOR(MemberName, DisplayName, ComponentTypeValue) \
    { DisplayName, FieldType::List, offsetof(ThisType, MemberName), getListFieldHandler(FieldType::ComponentRef), {}, {}, { ComponentTypeValue }, { FieldType::ComponentRef, getComponentRefFieldHandler() } }

// Non serializable fields macros

#define FIELD_GROUP_LABEL(DisplayName) \
    { DisplayName, FieldType::GroupLabel, 0, getGroupLabelFieldHandler(), {}, {}, {}, {}, true }

// These fields are internal, only meant to be used inside the FIELD_GROUP_COLLAPSE macro
#define INTERNAL_FIELD_GROUP_COLLAPSE_BEGIN(DisplayName) \
    { DisplayName, FieldType::GroupCollapseBegin, 0, nullptr, {}, {}, {}, {}, true }

#define INTERNAL_FIELD_GROUP_COLLAPSE_END() \
    { "", FieldType::GroupCollapseEnd, 0, nullptr, {}, {}, {}, {}, true }

#define FIELD_GROUP_COLLAPSE(DisplayName, ...) \
    INTERNAL_FIELD_GROUP_COLLAPSE_BEGIN(DisplayName), \
    __VA_ARGS__, \
    INTERNAL_FIELD_GROUP_COLLAPSE_END()
