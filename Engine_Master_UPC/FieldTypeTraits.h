#pragma once

#include <type_traits>
#include <string>

#include "FieldInfo.h"
#include "FieldHandlerRegistry.h"

class Component;
template<typename T> struct ComponentRef;

template<typename T>
constexpr FieldType fieldTypeOf()
{
    if constexpr (std::is_same_v<T, float>)                          return FieldType::Float;
    else if constexpr (std::is_same_v<T, int>)                       return FieldType::Int;
    else if constexpr (std::is_same_v<T, bool>)                      return FieldType::Bool;
    else if constexpr (std::is_same_v<T, Vector3>)                   return FieldType::Vec3;
    else if constexpr (std::is_same_v<T, std::string>)               return FieldType::String;
    else if constexpr (std::is_same_v<T, ComponentRef<Component>>) return FieldType::ComponentRef;
    else return FieldType::Float;
}

template<typename T>
const FieldHandler* fieldHandlerOf()
{
    if constexpr (std::is_same_v<T, float>)                          return getFloatFieldHandler();
    else if constexpr (std::is_same_v<T, int>)                       return getIntFieldHandler();
    else if constexpr (std::is_same_v<T, bool>)                      return getBoolFieldHandler();
    else if constexpr (std::is_same_v<T, Vector3>)                   return getVec3FieldHandler();
    else if constexpr (std::is_same_v<T, std::string>)               return getStringFieldHandler();
    else if constexpr (std::is_same_v<T, ComponentRef<Component>>) return getComponentRefFieldHandler();
    else return nullptr;
}

// Backward compatibility aliases
template<typename T>
constexpr FieldType scriptFieldTypeOf() { return fieldTypeOf<T>(); }

template<typename T>
const FieldHandler* scriptFieldHandlerOf() { return fieldHandlerOf<T>(); }
