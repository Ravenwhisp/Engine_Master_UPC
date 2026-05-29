#pragma once

#include <type_traits>
#include <string>

#include "ScriptFieldInfo.h"
#include "ScriptFieldHandlerRegistry.h"

class Component;
template<typename T> struct ScriptComponentRef;

template<typename T>
constexpr ScriptFieldType scriptFieldTypeOf()
{
    if constexpr (std::is_same_v<T, float>)                          return ScriptFieldType::Float;
    else if constexpr (std::is_same_v<T, int>)                       return ScriptFieldType::Int;
    else if constexpr (std::is_same_v<T, bool>)                      return ScriptFieldType::Bool;
    else if constexpr (std::is_same_v<T, Vector3>)                   return ScriptFieldType::Vec3;
    else if constexpr (std::is_same_v<T, std::string>)               return ScriptFieldType::String;
    else if constexpr (std::is_same_v<T, ScriptComponentRef<Component>>) return ScriptFieldType::ComponentRef;
    else return ScriptFieldType::Float;
}

template<typename T>
const ScriptFieldHandler* scriptFieldHandlerOf()
{
    if constexpr (std::is_same_v<T, float>)                          return getFloatFieldHandler();
    else if constexpr (std::is_same_v<T, int>)                       return getIntFieldHandler();
    else if constexpr (std::is_same_v<T, bool>)                      return getBoolFieldHandler();
    else if constexpr (std::is_same_v<T, Vector3>)                   return getVec3FieldHandler();
    else if constexpr (std::is_same_v<T, std::string>)               return getStringFieldHandler();
    else if constexpr (std::is_same_v<T, ScriptComponentRef<Component>>) return getComponentRefFieldHandler();
    else return nullptr;
}
