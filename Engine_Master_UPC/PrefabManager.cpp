#include "Globals.h"
#include "PrefabManager.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleScene.h"

#include "Prefab.h"
#include "AssetReference.h"

#include "UID.h"
#include "Scene.h"
#include "GameObject.h"
#include "PrefabInstanceComponent.h"
#include "Component.h"
#include "Transform.h"
#include "JsonArchive.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <filesystem>
#include <string>

using namespace rapidjson;
namespace fs = std::filesystem;

static constexpr int PREFAB_FORMAT_VERSION = 2;

namespace {

bool writeJsonFile(const Document& doc, const fs::path& path)
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

bool readJsonFile(const fs::path& path, Document& doc)
{
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file) return false;

    char buf[65536];
    FileReadStream is(file, buf, sizeof(buf));
    doc.ParseStream(is);
    fclose(file);
    return !doc.HasParseError();
}

PrefabInstanceComponent* getOrCreatePrefabComponent(GameObject* go)
{
    auto* comp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (!comp)
        comp = static_cast<PrefabInstanceComponent*>(go->AddComponentWithUID(ComponentType::PREFAB_INSTANCE, GenerateUID()));
    return comp;
}

void deserialiseTransform(const Value& node, GameObject* go)
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

void deserialiseComponents(const Value& node, GameObject* go)
{
    uint32_t componentCount = node.HasMember("ComponentCount") ? node["ComponentCount"].GetUint() : 0;
    if (componentCount == 0) return;

    for (uint32_t i = 0; i < componentCount; ++i)
    {
        std::string key = "Component_" + std::to_string(i);
        if (!node.HasMember(key.c_str())) continue;
        const Value& cn = node[key.c_str()];
        auto type = static_cast<ComponentType>(cn["ComponentType"].GetInt());
        Component* comp = go->AddComponentWithUID(type, GenerateUID());
        if (comp)
        {
            JsonArchive compArchive(ArchiveMode::Input);
            compArchive.setValue(cn);
            comp->serialize(compArchive);
        }
    }
}

void applyPrefabLink(const Value& node, GameObject* go)
{
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
}

} // anonymous namespace

PrefabManager::PrefabManager(ModuleAssets* moduleAssets) : m_moduleAssets(moduleAssets)
{
}

PrefabManager::~PrefabManager() = default;

bool PrefabManager::savePrefab(GameObject* go, const fs::path& savePath)
{
    if (!go || savePath.empty()) return false;

    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();
    doc.AddMember("SourcePath", Value(savePath.string().c_str(), alloc), alloc);
    doc.AddMember("Name", Value(savePath.stem().string().c_str(), alloc), alloc);
    doc.AddMember("Version", PREFAB_FORMAT_VERSION, alloc);

    auto* preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (preComp && preComp->isInstance() && preComp->getData().m_sourcePath != savePath)
        doc.AddMember("VariantOf", Value(preComp->getData().m_sourcePath.string().c_str(), alloc), alloc);

    JsonArchive goArchive;
    go->serialize(goArchive);
    Value goNode = goArchive.extractValue(doc.GetAllocator());
    doc.AddMember("GameObject", goNode, alloc);

    if (!writeJsonFile(doc, savePath)) return false;

    getOrCreatePrefabComponent(go);
    preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (preComp) preComp->getData().m_sourcePath = savePath;

    const UID existingUID = m_moduleAssets->getIndex().findUID(savePath);
    if (isValidUID(existingUID))
        m_moduleAssets->unload(AssetReference(existingUID));

    AssetReference ref(existingUID);
    m_moduleAssets->importAsset(savePath, ref);

    if (isValidUID(ref.m_uid) && preComp)
        preComp->getData().m_assetUID = ref.m_uid;

    return true;
}

bool PrefabManager::applyPrefab(const GameObject* go)
{
    auto* preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (!preComp || !preComp->isInstance()) return false;

    const fs::path& prefabPath = preComp->getData().m_sourcePath;

    if (!savePrefab(const_cast<GameObject*>(go), prefabPath))
        return false;

    Scene* scene = app->getModuleScene()->getScene();
    if (!scene) return true;

    for (GameObject* instance : scene->getAllGameObjects())
    {
        if (!instance || instance == go) continue;
        auto* instComp = instance->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
        if (instComp && instComp->getData().m_sourcePath == prefabPath)
            revertPrefab(instance, scene);
    }

    scene->markDirty();
    return true;
}

bool PrefabManager::revertPrefab(GameObject* go, Scene* scene)
{
    auto* preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (!preComp || !preComp->isInstance()) return false;

    const PrefabInstanceInfo& info = preComp->getData();

    Document doc;
    if (!readJsonFile(info.m_sourcePath, doc) || !doc.HasMember("GameObject"))
        return false;

    const Value& goNode = doc["GameObject"];
    const PrefabOverrideRecord savedOverrides = info.m_overrides;
    const int transformType = static_cast<int>(ComponentType::TRANSFORM);

    if (goNode.HasMember("Transform") && goNode["Transform"].IsObject())
    {
        Transform* tf = go->GetTransform();
        const Value& tfNode = goNode["Transform"];

        auto oit = savedOverrides.m_modifiedProperties.find(transformType);
        const auto* overrideSet = (oit != savedOverrides.m_modifiedProperties.end())
            ? &oit->second : nullptr;
        auto isOverridden = [&](const char* prop)
            { return overrideSet && overrideSet->count(prop) > 0; };

        if (!isOverridden("position") && tfNode.HasMember("position") && tfNode["position"].IsArray())
        {
            const auto& p = tfNode["position"];
            tf->setPosition(Vector3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat()));
        }
        if (!isOverridden("rotation") && tfNode.HasMember("rotation") && tfNode["rotation"].IsArray())
        {
            const auto& r = tfNode["rotation"];
            tf->setRotation(Quaternion(r[0].GetFloat(), r[1].GetFloat(), r[2].GetFloat(), r[3].GetFloat()));
        }
        if (!isOverridden("scale") && tfNode.HasMember("scale") && tfNode["scale"].IsArray())
        {
            const auto& s = tfNode["scale"];
            tf->setScale(Vector3(s[0].GetFloat(), s[1].GetFloat(), s[2].GetFloat()));
        }
        tf->markDirty();
    }

    uint32_t componentCount = goNode.HasMember("ComponentCount") ? goNode["ComponentCount"].GetUint() : 0;
    for (uint32_t i = 0; i < componentCount; ++i)
    {
        std::string key = "Component_" + std::to_string(i);
        if (!goNode.HasMember(key.c_str())) continue;

        const Value& cn = goNode[key.c_str()];
        if (!cn.HasMember("ComponentType")) continue;

        const int ct = cn["ComponentType"].GetInt();
        auto oit = savedOverrides.m_modifiedProperties.find(ct);
        if (oit != savedOverrides.m_modifiedProperties.end()
            && oit->second.count("properties") > 0)
            continue;

        Component* comp = go->GetComponent(static_cast<ComponentType>(ct));
        if (comp)
        {
            JsonArchive compArchive(ArchiveMode::Input);
            compArchive.setValue(cn);
            comp->serialize(compArchive);
        }
    }

    preComp->getData().m_overrides = savedOverrides;
    return true;
}

bool PrefabManager::createVariant(const fs::path& src, const fs::path& dst)
{
    if (src.empty() || dst.empty()) return false;

    Document doc;
    if (!readJsonFile(src, doc)) return false;
    auto& alloc = doc.GetAllocator();

    auto setOrAdd = [&](const char* key, const std::string& value)
        {
            if (doc.HasMember(key))
                doc[key].SetString(value.c_str(), alloc);
            else
                doc.AddMember(Value(key, alloc), Value(value.c_str(), alloc), alloc);
        };
    setOrAdd("SourcePath", dst.string());
    setOrAdd("Name", dst.stem().string());
    setOrAdd("VariantOf", src.string());

    return writeJsonFile(doc, dst);
}

GameObject* PrefabManager::spawnPrefab(const Prefab& prefab, Scene* scene)
{
    auto clone = prefab.spawnClone();
    if (!clone) return nullptr;
    GameObject* go = clone.get();

    auto* preComp = static_cast<PrefabInstanceComponent*>(go->AddComponentWithUID(ComponentType::PREFAB_INSTANCE, GenerateUID()));
    if (preComp)
    {
        preComp->getData().m_sourcePath = prefab.m_sourcePath;
        preComp->getData().m_assetUID = prefab.getUID();
    }

    if (scene)
    {
        scene->addGameObject(std::move(clone));
        go->init();
    }

    return go;
}

GameObject* PrefabManager::spawnPrefab(const fs::path& sourcePath, Scene* scene)
{
    if (!scene || sourcePath.empty()) return nullptr;

    auto asset = m_moduleAssets->loadAtPath<Prefab>(sourcePath);
    if (asset) return spawnPrefab(*asset, scene);

    Document doc;
    if (!readJsonFile(sourcePath, doc) || !doc.HasMember("GameObject")) return nullptr;

    GameObject* go = createFromJSON(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    auto* preComp = getOrCreatePrefabComponent(go);
    if (preComp)
        preComp->getData().m_sourcePath = sourcePath;

    return go;
}

// ── Static tree-building helpers ──

GameObject* PrefabManager::createFromJSON(const Value& node, Scene* scene, GameObject* parent)
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

    applyPrefabLink(node, go);
    deserialiseTransform(node, go);
    deserialiseComponents(node, go);

    if (node.HasMember("Children") && node["Children"].IsArray())
    {
        for (SizeType i = 0; i < node["Children"].Size(); ++i)
            createFromJSON(node["Children"][i], scene, go);
    }

    if (!parent) go->init();
    return go;
}

GameObject* PrefabManager::createFromJSON(const Value& node, GameObject* parent)
{
    if (!node.IsObject()) return nullptr;
    GameObject* go = new GameObject(GenerateUID(), GenerateUID());
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
    }

    applyPrefabLink(node, go);
    deserialiseTransform(node, go);
    deserialiseComponents(node, go);

    if (node.HasMember("Children") && node["Children"].IsArray())
    {
        for (SizeType i = 0; i < node["Children"].Size(); ++i)
            createFromJSON(node["Children"][i], go);
    }

    if (!parent) go->init();
    return go;
}
