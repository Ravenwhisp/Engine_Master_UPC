#include "Globals.h"
#include "PrefabAsset.h"

PrefabAsset::PrefabAsset(UID id, GameObject* gameObject) : Asset(id, AssetType::PREFAB)
{
    m_gameObjectInstance.reset(gameObject);
    //If it already had a PrefabAsset this means that this PrefabAsset is a variant of that Prefab
    m_variant = gameObject->GetPrefabInfo().m_assetUID;
    m_gameObjectInstance->setPrefabInfo(m_uid);
}

void PrefabAsset::revert(GameObject* gameObject)
{
    auto id = gameObject->GetID();
    gameObject = m_gameObjectInstance.get();
    gameObject->SetID(id);
}

std::unique_ptr<GameObject> PrefabAsset::spawnPrefab()
{
    auto instance = m_gameObjectInstance.get()->clone();
    instance->SetID(GenerateUID());
    return instance;

