#include "Globals.h"
#include "PrefabAsset.h"
#include "GameObject.h"
#include "Transform.h"

PrefabAsset::PrefabAsset(UID id, std::unique_ptr<GameObject> gameObject) : Asset(id, AssetType::PREFAB)
{
    setGameObject(std::move(gameObject));
}

void PrefabAsset::revert(GameObject* gameObject)
{
    if (!gameObject || !m_gameObjectInstance) return;

    // Preserve live-object identity before we overwrite anything.
    const UID                   liveUID = gameObject->GetID();
    const UID                   assetUID = gameObject->GetPrefabInfo().m_assetUID;
    Transform* liveParent = gameObject->GetTransform()->getRoot();

    gameObject->SetName(m_gameObjectInstance->GetName());
    gameObject->SetActive(m_gameObjectInstance->GetActive());
    gameObject->SetStatic(m_gameObjectInstance->GetStatic());
    gameObject->SetLayer(m_gameObjectInstance->GetLayer());
    gameObject->SetTag(m_gameObjectInstance->GetTag());

    const Transform* tmplTf = m_gameObjectInstance->GetTransform();
    Transform* liveTf = gameObject->GetTransform();
    liveTf->setPosition(tmplTf->getPosition());
    liveTf->setRotation(tmplTf->getRotation());
    liveTf->setScale(tmplTf->getScale());

    {
        std::vector<Component*> toRemove;
        for (Component* c : gameObject->GetAllComponents())
        {
            if (c->getType() != ComponentType::TRANSFORM)
                toRemove.push_back(c);
        }
        for (Component* c : toRemove)
            gameObject->RemoveComponent(c);
    }

    for (Component* tmplComp : m_gameObjectInstance->GetAllComponents())
    {
        if (tmplComp->getType() == ComponentType::TRANSFORM) continue;

        auto cloned = tmplComp->clone(gameObject);
        if (cloned)
        {
            cloned->init();
            gameObject->AddClonedComponent(std::move(cloned));
        }
    }

    gameObject->SetID(liveUID);
    gameObject->GetPrefabInfo().m_assetUID = assetUID;
    gameObject->GetPrefabInfo().m_overrides.clear();
    gameObject->GetTransform()->setRoot(liveParent);
}

void PrefabAsset::setGameObject(std::unique_ptr<GameObject> gameObject)
{
    m_variant = gameObject->GetPrefabInfo().m_assetUID;
    m_gameObjectInstance = std::move(gameObject);
    m_gameObjectInstance->setPrefabInfo(m_uid);
}

std::unique_ptr<GameObject> PrefabAsset::spawnPrefab()
{
    auto instance = m_gameObjectInstance->clone();
    instance->SetID(GenerateUID());
    instance->GetPrefabInfo().m_overrides.clear();
    return instance;
}

