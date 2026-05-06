#include "Globals.h"
#include "PrefabSerializer.h"

#include "GameObject.h"
#include "Scene.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Skin.h"
#include "Component.h"
#include "ComponentType.h"
#include "PrefabAsset.h"

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <filesystem>

using namespace rapidjson;
namespace fs = std::filesystem;

static constexpr int PREFAB_FORMAT_VERSION = 2;

void PrefabSerializer::serialiseTransform(const GameObject* go, Value& out, Document::AllocatorType& alloc)
{
    const Transform* tf = go->GetTransform();
    const Vector3& pos = tf->getPosition();
    const Quaternion rot = tf->getRotation();
    const Vector3& sc = tf->getScale();

    Value posArr(kArrayType);
    posArr.PushBack(pos.x, alloc).PushBack(pos.y, alloc).PushBack(pos.z, alloc);
    Value rotArr(kArrayType);
    rotArr.PushBack(rot.x, alloc).PushBack(rot.y, alloc).PushBack(rot.z, alloc).PushBack(rot.w, alloc);
    Value scArr(kArrayType);
    scArr.PushBack(sc.x, alloc).PushBack(sc.y, alloc).PushBack(sc.z, alloc);

    Value tfNode(kObjectType);
    tfNode.AddMember("position", posArr, alloc);
    tfNode.AddMember("rotation", rotArr, alloc);
    tfNode.AddMember("scale", scArr, alloc);
    out.AddMember("Transform", tfNode, alloc);
}

void PrefabSerializer::serialiseComponents(const GameObject* go, Value& out, Document::AllocatorType& alloc)
{
    Value    compArray(kArrayType);
    Document helperDoc;
    helperDoc.SetObject();

    for (Component* comp : go->GetAllComponents())
    {
        if (comp->getType() == ComponentType::TRANSFORM) continue;

        Value dataCopy;
        dataCopy.CopyFrom(comp->getJSON(helperDoc), alloc);

        Value compNode(kObjectType);
        compNode.AddMember("Type", static_cast<int>(comp->getType()), alloc);
        compNode.AddMember("Data", dataCopy, alloc);
        compArray.PushBack(compNode, alloc);
    }
    out.AddMember("Components", compArray, alloc);
}

void PrefabSerializer::serialiseNodeInto(const GameObject* go, Value& out, Document::AllocatorType& alloc)
{
    out.SetObject();
    out.AddMember("Name", Value(go->GetName().c_str(), alloc), alloc);
    out.AddMember("Active", go->GetActive(), alloc);
    out.AddMember("Tag", Value(TagToString(go->GetTag()), alloc), alloc);
    out.AddMember("Layer", Value(LayerToString(go->GetLayer()), alloc), alloc);

    const PrefabInstanceInfo& info = go->GetPrefabInfo();
    if (info.isInstance())
    {
        Value prefabLink(kObjectType);
        prefabLink.AddMember("SourcePath", Value(info.m_sourcePath.string().c_str(), alloc), alloc);
        prefabLink.AddMember("AssetUID", Value(info.m_assetUID.c_str(), alloc), alloc);
        out.AddMember("PrefabLink", prefabLink, alloc);
    }

    serialiseTransform(go, out, alloc);
    serialiseComponents(go, out, alloc);

    Value childrenArray(kArrayType);
    for (GameObject* child : go->GetTransform()->getAllChildren())
    {
        Value childNode;
        serialiseNodeInto(child, childNode, alloc);
        childrenArray.PushBack(childNode, alloc);
    }
    out.AddMember("Children", childrenArray, alloc);
}

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
    MD5Hash legacySkinAssetId = INVALID_ASSET_ID;

    for (SizeType i = 0; i < node["Components"].Size(); ++i)
    {
        const Value& cn = node["Components"][i];
        auto type = static_cast<ComponentType>(cn["Type"].GetInt());

        if (type == ComponentType::SKIN)
        {
            const Value* data = nullptr;

            if (cn.HasMember("Data") && cn["Data"].IsObject())
            {
                data = &cn["Data"];
            }
            else
            {
                data = &cn;
            }

            if (data && data->HasMember("SkinAssetId") && (*data)["SkinAssetId"].IsString())
            {
                legacySkinAssetId = (*data)["SkinAssetId"].GetString();
            }

            continue;
        }

        Component* comp = go->AddComponentWithUID(type, GenerateUID());
        if (comp && cn.HasMember("Data") && cn["Data"].IsObject())
            comp->deserializeJSON(cn["Data"]);
    }


    MeshRenderer* meshRenderer = go->GetComponentAs<MeshRenderer>(ComponentType::MODEL);

    if (meshRenderer)
    {
        if (legacySkinAssetId != INVALID_ASSET_ID && !meshRenderer->hasSkin())
        {
            meshRenderer->ensureSkin().setSkinReference(legacySkinAssetId);
        }
        else if (meshRenderer->getSkinReference() != INVALID_ASSET_ID && !meshRenderer->hasSkin())
        {
            meshRenderer->ensureSkin().setSkinReference(meshRenderer->getSkinReference());
        }
    }

}

bool PrefabSerializer::writeDocument(Document& doc, const fs::path& path)
{
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    if (ec) return false;

    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;

    char buf[65536];
    FileWriteStream os(file, buf, sizeof(buf));
    PrettyWriter<FileWriteStream> writer(os);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);
    fclose(file);
    return true;
}

bool PrefabSerializer::readDocument(const fs::path& path, Document& doc)
{
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file) return false;

    char buf[65536];
    FileReadStream is(file, buf, sizeof(buf));
    doc.ParseStream(is);
    fclose(file);
    return !doc.HasParseError();
}

bool PrefabSerializer::loadDocument(const fs::path& path, Document& doc)
{
    return readDocument(path, doc)
        && doc.HasMember("GameObject")
        && doc["GameObject"].IsObject();
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

    const PrefabInstanceInfo& info = go->GetPrefabInfo();
    if (info.isInstance() && info.m_sourcePath != savePath)
        doc.AddMember("VariantOf", Value(info.m_sourcePath.string().c_str(), alloc), alloc);
}

std::string PrefabSerializer::buildPrefabJSON(const GameObject* go, const fs::path& savePath)
{
    if (!go || savePath.empty()) return {};

    Document doc;
    buildDocumentHeader(doc, go, savePath);

    Value goNode;
    serialiseNodeInto(go, goNode, doc.GetAllocator());
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
        PrefabInstanceInfo& info = go->GetPrefabInfo();
        if (pl.HasMember("SourcePath") && pl["SourcePath"].IsString())
            info.m_sourcePath = pl["SourcePath"].GetString();
        if (pl.HasMember("AssetUID") && pl["AssetUID"].IsString())
            info.m_assetUID = pl["AssetUID"].GetString();
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