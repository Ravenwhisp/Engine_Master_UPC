#pragma once

#include "AssetReference.h"
#include <memory>

class DataContainer;

template<typename T = void>
struct AssetRef
{
    AssetReference m_ref;
    std::shared_ptr<T> m_data;

    T* get() const
    {
        return m_data.get();
    }
};

using PrefabRef = AssetRef<struct Prefab>;
using SceneRef = AssetRef<struct Scene>;
using MaterialRef = AssetRef<struct BasicMaterial>;
using DataContainerRef = AssetRef<DataContainer>;
