#pragma once

#include "AssetId.h"
#include <memory>

class DataContainer;

template<typename T = void>
struct AssetReference
{
    AssetId m_id;
    std::shared_ptr<T> m_data;

    T* get() const
    {
        return m_data.get();
    }

    T* operator->() const
    {
        return m_data.get();
    }

    explicit operator bool() const
    {
        return m_data != nullptr;
    }

    bool operator!() const
    {
        return !m_data;
    }

    bool operator==(const AssetReference& o) const
    {
        return m_id == o.m_id;
    }

    bool operator!=(const AssetReference& o) const
    {
        return !(*this == o);
    }

    void serialize(IArchive& archive)
    {
        m_id.serialize(archive);
    }
};

using PrefabRef = AssetReference<struct Prefab>;
using SceneRef = AssetReference<struct Scene>;
using MaterialRef = AssetReference<struct BasicMaterial>;
using DataContainerRef = AssetReference<DataContainer>;
