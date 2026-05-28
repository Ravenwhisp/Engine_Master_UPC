#pragma once
#include "Asset.h"
#include "GameObject.h"
#include "IArchive.h"
#include "ISerializable.h"
#include "UID.h"
#include <filesystem>
#include <string>
#include "Transform.h"


class Prefab : public Asset, public GameObject
{
public:
    Prefab() : Asset(), GameObject(GenerateUID(), GenerateUID()) {}

    explicit Prefab(AssetReference& id)
        : Asset(id, AssetType::PREFAB)
        , GameObject(GenerateUID(), GenerateUID())
    {
    }

    std::filesystem::path m_sourcePath;

    void serialize(IArchive& archive) override
    {
        std::string pathStr = m_sourcePath.string();
        archive.serialize(pathStr);
        if (archive.mode() == ArchiveMode::Input)
            m_sourcePath = pathStr;

        GameObject::serialize(archive);
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

        adoptComponentsFrom(source);
        adoptChildrenFrom(source);
    }
};
