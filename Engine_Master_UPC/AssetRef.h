#pragma once

#include "AssetReference.h"

template<typename>
struct AssetRef
{
    AssetReference m_ref;
};

using PrefabRef = AssetRef<struct Prefab>;
using SceneRef = AssetRef<struct Scene>;
using MaterialRef = AssetRef<struct BasicMaterial>;
