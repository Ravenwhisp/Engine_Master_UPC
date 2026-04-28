#pragma once
#include "Asset.h"
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

    explicit PrefabAsset(UID id, GameObject* gameObject) : Asset(id, AssetType::PREFAB)
    {
        applyGameObject(gameObject);
    }

    void applyGameObject(GameObject* gameobject)
    {
        m_gameObjectInstance.reset(gameobject);
    }
    GameObject* getGameObjectInstance() { return m_gameObjectInstance.get(); }

#pragma region Serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::base_class<Asset>(this), m_gameObjectInstance);
    }
#pragma endregion

private:
    std::unique_ptr<GameObject> m_gameObjectInstance;
};

CEREAL_REGISTER_TYPE(PrefabAsset)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Asset, PrefabAsset)