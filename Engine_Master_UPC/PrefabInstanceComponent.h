#pragma once
#include "Component.h"
#include "PrefabInstance.h"
#include "IArchive.h"

class PrefabInstanceComponent : public Component
{
public:
    PrefabInstanceComponent(UID id, GameObject* owner)
        : Component(id, ComponentType::PREFAB_INSTANCE, owner) {}

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
        std::string src = m_data.m_sourcePath.string();
        archive.serialize(src, "sourcePath");
        if (archive.mode() == ArchiveMode::Input)
            m_data.m_sourcePath = src;

        archive.serialize(m_data.m_assetUID, "assetUID");
    }

    rapidjson::Value getJSON(rapidjson::Document& domTree) override
    {
        rapidjson::Value obj(rapidjson::kObjectType);
        auto& alloc = domTree.GetAllocator();
        obj.AddMember("UID", m_uuid, alloc);
        obj.AddMember("SourcePath", rapidjson::Value(m_data.m_sourcePath.string().c_str(), alloc), alloc);
        if (m_data.m_assetUID != INVALID_UID)
            obj.AddMember("AssetUID", m_data.m_assetUID, alloc);
        return obj;
    }

    bool deserializeJSON(const rapidjson::Value& obj) override
    {
        if (obj.HasMember("SourcePath") && obj["SourcePath"].IsString())
            m_data.m_sourcePath = obj["SourcePath"].GetString();
        if (obj.HasMember("AssetUID") && obj["AssetUID"].IsUint64())
            m_data.m_assetUID = obj["AssetUID"].GetUint64();
        return true;
    }

private:
    PrefabInstanceInfo m_data;
};
