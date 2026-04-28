#pragma once
#include "Asset.h"
#include "GameObject.h"
#include "UID.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PrefabOverrideRecord
{
    std::unordered_map<int, std::unordered_set<std::string>> m_modifiedProperties;
    std::vector<int> m_addedComponentTypes;
    std::vector<int> m_removedComponentTypes;

    bool isEmpty() const
    {
        return m_modifiedProperties.empty()
            && m_addedComponentTypes.empty()
            && m_removedComponentTypes.empty();
    }

    void clear()
    {
        m_modifiedProperties.clear();
        m_addedComponentTypes.clear();
        m_removedComponentTypes.clear();
    }
};


class PrefabAsset : public Asset
{
public:
    friend class ImporterPrefab;
    friend class ImporterGltf;

    PrefabAsset() = default;
    explicit PrefabAsset(UID id) : Asset(id, AssetType::PREFAB)
    {
    }

    explicit PrefabAsset(UID id, GameObject* gameObject);

    std::unique_ptr<GameObject> spawnPrefab();

    void revert(GameObject* gameObject);

    bool isVariant() { return isValidUID(m_variant); }

    //A function that given two GameObjects or PrefabAssets it returns the OverridedRecord

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