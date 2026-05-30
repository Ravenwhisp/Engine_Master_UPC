#pragma once
#include "Component.h"
#include "PrefabInstance.h"
#include "IArchive.h"

class PrefabInstanceComponent : public Component
{
public:
    PrefabInstanceComponent(UID id, GameObject* owner) : Component(id, ComponentType::PREFAB_INSTANCE, owner) {}

    std::unique_ptr<Component> clone(GameObject* newOwner) const override
    {
        auto c = std::make_unique<PrefabInstanceComponent>(GenerateUID(), newOwner);
        c->m_data = m_data;
        return c;
    }

    PrefabInstanceInfo& getData() { return m_data; }
    const PrefabInstanceInfo& getData() const { return m_data; }

    bool isInstance() const { return m_data.isInstance(); }

    void serialize(IArchive& archive) override
    {
        if (archive.mode() == ArchiveMode::Output)
        {
            archive.serialize(m_uuid, "UID");
            uint32_t type = static_cast<uint32_t>(ComponentType::PREFAB_INSTANCE);
            archive.serialize(type, "ComponentType");
        }

        std::string src = m_data.m_sourcePath.string();
        archive.serialize(src, "sourcePath");
        if (archive.mode() == ArchiveMode::Input)
            m_data.m_sourcePath = src;

        archive.serialize(m_data.m_assetUID, "assetUID");
    }

private:
    PrefabInstanceInfo m_data;
};
