#pragma once
#include "Asset.h"
#include "UID.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "PrefabInfo.h"
#include "GameObject.h"

class PrefabAsset : public Asset
{
public:
    friend class ImporterPrefab;
    friend class ImporterGltf;

    PrefabAsset() = default;
    explicit PrefabAsset(UID id) : Asset(id, AssetType::PREFAB)
    {
    }

    explicit PrefabAsset(UID id, std::unique_ptr<GameObject> gameObject);

    std::unique_ptr<GameObject> spawnPrefab();

    void revert(GameObject* gameObject);

    bool isVariant() const { return isValidUID(m_variant); }

    //A function that given two GameObjects or PrefabAssets it returns the OverridedRecord
    void setGameObject(std::unique_ptr<GameObject> gameObject);
#pragma region Serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::base_class<Asset>(this), m_gameObjectInstance, m_variant);
    }
#pragma endregion

private:
    std::unique_ptr<GameObject> m_gameObjectInstance;
    UID m_variant;
};

CEREAL_REGISTER_TYPE(PrefabAsset)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Asset, PrefabAsset)