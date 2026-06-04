#pragma once
#include "Asset.h"
#include "GameObject.h"
#include "IArchive.h"
#include "ISerializable.h"
#include "UID.h"
#include "Component.h"
#include "Transform.h"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>

class Prefab : public Asset, public GameObject
{
public:

    explicit Prefab(AssetReference& id)
        : Asset(id, AssetType::PREFAB)
        , GameObject(GenerateUID(), GenerateUID())
    {
    }

    std::filesystem::path m_sourcePath;

    void serialize(IArchive& archive) override
    {
        std::string pathStr = m_sourcePath.string();
        archive.serialize(pathStr, "sourcePath");
        if (archive.mode() == ArchiveMode::Input)
        {
            m_sourcePath = pathStr;
        }

        GameObject::serialize(archive);
        serializeChildren(archive, this);
    }

    std::unique_ptr<GameObject> spawnClone() const
    {
        return GameObject::clone();
    }

    void buildFrom(GameObject* source)
    {
        SetName(source->GetName());
        SetActive(source->GetActive());
        SetStatic(source->GetStatic());
        SetLayer(source->GetLayer());
        SetTag(source->GetTag());

        GetTransform()->setPosition(source->GetTransform()->getPosition());
        GetTransform()->setRotation(source->GetTransform()->getRotation());
        GetTransform()->setScale(source->GetTransform()->getScale());

        for (Component* comp : source->GetAllComponents())
        {
            if (comp->getType() != ComponentType::TRANSFORM &&
                comp->getType() != ComponentType::PREFAB_INSTANCE)
            {
                auto cloned = comp->clone(this);
                if (cloned)
                {
                    AddClonedComponent(std::move(cloned));
                }
            }
        }

        m_ownedChildren.clear();
        cloneChildTree(source, this);
    }

private:
    void cloneChildTree(GameObject* sourceParent, GameObject* dstParent)
    {
        for (GameObject* child : sourceParent->GetTransform()->getAllChildren())
        {
            auto childClone = std::make_unique<GameObject>(GenerateUID(), GenerateUID());
            GameObject* rawChild = childClone.get();

            rawChild->SetName(child->GetName());
            rawChild->SetActive(child->GetActive());
            rawChild->SetStatic(child->GetStatic());
            rawChild->SetLayer(child->GetLayer());
            rawChild->SetTag(child->GetTag());

            rawChild->GetTransform()->setPosition(child->GetTransform()->getPosition());
            rawChild->GetTransform()->setRotation(child->GetTransform()->getRotation());
            rawChild->GetTransform()->setScale(child->GetTransform()->getScale());

            for (Component* comp : child->GetAllComponents())
            {
                if (comp->getType() != ComponentType::TRANSFORM)
                {
                    auto cloned = comp->clone(rawChild);
                    if (cloned)
                    {
                        rawChild->AddClonedComponent(std::move(cloned));
                    }
                }
            }

            rawChild->GetTransform()->setRoot(dstParent->GetTransform());
            dstParent->GetTransform()->addChild(rawChild);

            m_ownedChildren.push_back(std::move(childClone));

            cloneChildTree(child, rawChild);
        }
    }

    void serializeChildren(IArchive& archive, GameObject* parent)
    {
        if (archive.mode() == ArchiveMode::Output)
        {
            uint32_t childCount = static_cast<uint32_t>(parent->GetTransform()->getAllChildren().size());
            archive.beginArray(childCount, "Children");
            for (uint32_t i = 0; i < childCount; ++i)
            {
                archive.beginObject();
                GameObject* child = parent->GetTransform()->getAllChildren()[i];
                child->serialize(archive);
                serializeChildren(archive, child);
                archive.endObject();
            }
            archive.endArray();
        }
        else
        {
            uint32_t childCount = 0;
            archive.beginArray(childCount, "Children");
            for (uint32_t i = 0; i < childCount; ++i)
            {
                archive.beginObject();
                auto childGo = std::make_unique<GameObject>(GenerateUID(), GenerateUID());
                GameObject* rawChild = childGo.get();
                rawChild->GetTransform()->setRoot(parent->GetTransform());
                parent->GetTransform()->addChild(rawChild);
                m_ownedChildren.push_back(std::move(childGo));
                rawChild->serialize(archive);
                serializeChildren(archive, rawChild);
                archive.endObject();
            }
            archive.endArray();
        }
    }
};
