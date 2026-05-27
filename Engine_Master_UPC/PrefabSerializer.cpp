#include "Globals.h"
#include "PrefabSerializer.h"

#include "GameObject.h"
#include "PrefabInstanceComponent.h"
#include "Scene.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Skin.h"
#include "Component.h"
#include "ComponentType.h"
#include "Prefab.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <filesystem>

using namespace rapidjson;
namespace fs = std::filesystem;

static constexpr int PREFAB_FORMAT_VERSION = 2;

void PrefabSerializer::deserialiseTransform(const Value& node, GameObject* go)
{
    if (!node.HasMember("Transform") || !node["Transform"].IsObject()) return;
    Transform* tf = go->GetTransform();
    const Value& tfNode = node["Transform"];

    if (tfNode.HasMember("position") && tfNode["position"].IsArray())
    {
        const auto& p = tfNode["position"];
        tf->setPosition(Vector3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat()));
    }
    if (tfNode.HasMember("rotation") && tfNode["rotation"].IsArray())
    {
        const auto& r = tfNode["rotation"];
        tf->setRotation(Quaternion(r[0].GetFloat(), r[1].GetFloat(), r[2].GetFloat(), r[3].GetFloat()));
    }
    if (tfNode.HasMember("scale") && tfNode["scale"].IsArray())
    {
        const auto& s = tfNode["scale"];
        tf->setScale(Vector3(s[0].GetFloat(), s[1].GetFloat(), s[2].GetFloat()));
    }
}

void PrefabSerializer::deserialiseComponents(const Value& node, GameObject* go)
{
    if (!node.HasMember("Components") || !node["Components"].IsArray()) return;

    for (SizeType i = 0; i < node["Components"].Size(); ++i)
    {
        const Value& cn = node["Components"][i];
        auto type = static_cast<ComponentType>(cn["Type"].GetInt());

        Component* comp = go->AddComponentWithUID(type, GenerateUID());
        if (comp && cn.HasMember("Data") && cn["Data"].IsObject())
        {
            comp->deserializeJSON(cn["Data"]);
        }
    }
}

void PrefabSerializer::buildDocumentHeader(Document& doc, const GameObject* go, const fs::path& savePath)
{
    doc.SetObject();
    auto& alloc = doc.GetAllocator();
    const std::string pathStr = savePath.string();
    const std::string name = savePath.stem().string();

    doc.AddMember("SourcePath", Value(pathStr.c_str(), alloc), alloc);
    doc.AddMember("Name", Value(name.c_str(), alloc), alloc);
    doc.AddMember("Version", PREFAB_FORMAT_VERSION, alloc);

    auto* preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (preComp && preComp->isInstance() && preComp->getData().m_sourcePath != savePath)
    {
        doc.AddMember("VariantOf", Value(preComp->getData().m_sourcePath.string().c_str(), alloc), alloc);
    }
}

std::string PrefabSerializer::buildPrefabJSON(const GameObject* go, const fs::path& savePath)
{
    if (!go || savePath.empty()) return {};

    Document doc;
    buildDocumentHeader(doc, go, savePath);

    // Use GameObject::getJSON() which already serializes name, active, tag, layer,
    // transform, prefab info, components, and children recursively.
    Value goNode = const_cast<GameObject*>(go)->getJSON(doc);
    doc.AddMember("GameObject", goNode, doc.GetAllocator());

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}

GameObject* PrefabSerializer::deserialiseNode(const Value& node, Scene* scene, GameObject* parent)
{
    if (!node.IsObject()) return nullptr;

    GameObject* go = scene->createGameObjectWithUID(GenerateUID(), GenerateUID());
    if (!go) return nullptr;

    go->SetName(node.HasMember("Name") ? node["Name"].GetString() : "Unnamed");
    go->SetActive(node.HasMember("Active") ? node["Active"].GetBool() : true);
    if (node.HasMember("Tag") && node["Tag"].IsString())
        go->SetTag(StringToTag(node["Tag"].GetString()));

    if (node.HasMember("Layer") && node["Layer"].IsString())
        go->SetLayer(StringToLayer(node["Layer"].GetString()));
    if (parent)
    {
        go->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(go);
        scene->removeFromRootList(go);
    }

    if (node.HasMember("PrefabLink") && node["PrefabLink"].IsObject())
    {
        const Value& pl = node["PrefabLink"];
        auto* preComp = static_cast<PrefabInstanceComponent*>(go->AddComponentWithUID(ComponentType::PREFAB_INSTANCE, GenerateUID()));
        if (preComp)
        {
            auto& data = preComp->getData();
            if (pl.HasMember("SourcePath") && pl["SourcePath"].IsString())
                data.m_sourcePath = pl["SourcePath"].GetString();
            if (pl.HasMember("AssetUID") && pl["AssetUID"].IsUint64())
                data.m_assetUID = pl["AssetUID"].GetUint64();
        }
    }

    deserialiseTransform(node, go);
    deserialiseComponents(node, go);

    if (node.HasMember("Children") && node["Children"].IsArray())
    {
        for (SizeType i = 0; i < node["Children"].Size(); ++i)
            deserialiseNode(node["Children"][i], scene, go);
    }

    if (!parent)
    {
        go->init();
    }

    return go;
}

GameObject* PrefabSerializer::deserialiseNode(const Value& node, GameObject* parent) {
    if (!node.IsObject()) return nullptr;
    GameObject* go = new GameObject(GenerateUID(), GenerateUID());
    if (!go) return nullptr;
    go->SetName(node.HasMember("Name") ? node["Name"].GetString() : "Unnamed");
    go->SetActive(node.HasMember("Active") ? node["Active"].GetBool() : true);
    if (node.HasMember("Tag") && node["Tag"].IsString())
        go->SetTag(StringToTag(node["Tag"].GetString()));
    if (node.HasMember("Layer") && node["Layer"].IsString())
        go->SetLayer(StringToLayer(node["Layer"].GetString()));
    if (parent) {
        go->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(go);
    }
    if (node.HasMember("PrefabLink") && node["PrefabLink"].IsObject()) {
        const Value& pl = node["PrefabLink"];
        auto* preComp = static_cast<PrefabInstanceComponent*>(go->AddComponentWithUID(ComponentType::PREFAB_INSTANCE, GenerateUID()));
        if (preComp)
        {
            auto& data = preComp->getData();
            if (pl.HasMember("SourcePath") && pl["SourcePath"].IsString())
                data.m_sourcePath = pl["SourcePath"].GetString();
            if (pl.HasMember("AssetUID") && pl["AssetUID"].IsUint64())
                data.m_assetUID = pl["AssetUID"].GetUint64();
        }
    }
    deserialiseTransform(node, go);
    deserialiseComponents(node, go);
    if (node.HasMember("Children") && node["Children"].IsArray()) {
        for (SizeType i = 0; i < node["Children"].Size(); ++i)
            deserialiseNode(node["Children"][i], go);
    }
    if (!parent) {
        go->init();
    }
    return go;
}
