#pragma once

#include <type_traits>
#include <string>

#include "FieldInfo.h"
#include "FieldHandlerRegistry.h"

class Component;
template<typename T> struct ComponentRef;
template<typename T = void> struct AssetReference;

template<typename T> struct is_asset_ref : std::false_type {};
template<typename T> struct is_asset_ref<AssetReference<T>> : std::true_type {};
template<typename T> inline constexpr bool is_asset_ref_v = is_asset_ref<T>::value;

template<typename T>
constexpr FieldType fieldTypeOf()
{
    if constexpr (std::is_same_v<T, float>)                          return FieldType::Float;
    else if constexpr (std::is_same_v<T, int>)                       return FieldType::Int;
    else if constexpr (std::is_same_v<T, bool>)                      return FieldType::Bool;
    else if constexpr (std::is_same_v<T, Vector3>)                   return FieldType::Vec3;
    else if constexpr (std::is_same_v<T, std::string>)               return FieldType::String;
    else if constexpr (std::is_same_v<T, ComponentRef<Component>>) return FieldType::ComponentRef;
    else if constexpr (is_asset_ref_v<T>)                         return FieldType::AssetRef;
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
    else if constexpr (is_asset_ref_v<T>)                          return getAssetRefFieldHandler();
    else return nullptr;
}

// Backward compatibility aliases
template<typename T>
constexpr FieldType scriptFieldTypeOf() { return fieldTypeOf<T>(); }

template<typename T>
const FieldHandler* scriptFieldHandlerOf() { return fieldHandlerOf<T>(); }
