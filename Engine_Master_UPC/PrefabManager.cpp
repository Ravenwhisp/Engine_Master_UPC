#include "Globals.h"
#include "PrefabManager.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleScene.h"

#include "PrefabAsset.h"
#include "AssetReference.h"
#include "PrefabSerializer.h"

#include "UID.h"
#include "Scene.h"
#include "GameObject.h"
#include "Component.h"
#include "Transform.h"

#include <rapidjson/document.h>
#include <filesystem>
#include <string>

using namespace rapidjson;
namespace fs = std::filesystem;

PrefabManager::PrefabManager(ModuleAssets* moduleAssets) : m_moduleAssets(moduleAssets)
{
}

PrefabManager::~PrefabManager() = default;

bool PrefabManager::savePrefab(GameObject* go, const fs::path& savePath)
{
    if (!go || savePath.empty()) return false;

    Document doc;
    const std::string json = PrefabSerializer::buildPrefabJSON(go, savePath);
    doc.Parse(json.c_str());
    if (!PrefabSerializer::writeDocument(doc, savePath)) return false;

    go->GetPrefabInfo().m_sourcePath = savePath;

    // Evict stale cached version, then reimport.
    const UID existingUID = m_moduleAssets->getIndex().findUID(savePath);
    if (isValidUID(existingUID))
    {
        m_moduleAssets->unload(AssetReference(existingUID));
    }

    AssetReference ref(existingUID);
    m_moduleAssets->importAsset(savePath, ref);

    if (auto asset = m_moduleAssets->load<PrefabAsset>(ref))
    {
        asset->getData().m_json = json;
    }

    return true;
}

bool PrefabManager::applyPrefab(const GameObject* go)
{
    const PrefabInstanceInfo& info = go->GetPrefabInfo();
    if (!info.isInstance()) return false;

    if (!savePrefab(const_cast<GameObject*>(go), info.m_sourcePath))
        return false;

    Scene* scene = app->getModuleScene()->getScene();
    if (!scene) return true;

    const fs::path& prefabPath = info.m_sourcePath;
    for (GameObject* instance : scene->getAllGameObjects())
    {
        if (!instance)                                              continue;
        if (!instance->GetPrefabInfo().isInstance())               continue;
        if (instance->GetPrefabInfo().m_sourcePath != prefabPath)  continue;
        if (instance == go)                                         continue;
        revertPrefab(instance, scene);
    }

    scene->markDirty();
    return true;
}

bool PrefabManager::revertPrefab(GameObject* go, Scene* scene)
{
    PrefabInstanceInfo& info = go->GetPrefabInfo();
    if (!info.isInstance()) return false;

    Document doc;
    bool loaded = false;

    if (isValidUID(info.m_assetUID))
    {
        AssetReference ref(info.m_assetUID);
        auto asset = m_moduleAssets->load<PrefabAsset>(ref);
        if (asset && !asset->getJSON().empty())
        {
            doc.Parse(asset->getJSON().c_str());
            loaded = !doc.HasParseError() && doc.HasMember("GameObject");
        }
    }
    if (!loaded)
    {
        loaded = PrefabSerializer::loadDocument(info.m_sourcePath, doc);
    }
    if (!loaded) return false;

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

    if (goNode.HasMember("Components") && goNode["Components"].IsArray())
    {
        for (SizeType i = 0; i < goNode["Components"].Size(); ++i)
        {
            const Value& cn = goNode["Components"][i];
            if (!cn.HasMember("Type") || !cn.HasMember("Data")) continue;

            const int ct = cn["Type"].GetInt();
            auto oit = savedOverrides.m_modifiedProperties.find(ct);
            if (oit != savedOverrides.m_modifiedProperties.end()
                && oit->second.count("properties") > 0)
                continue;

            Component* comp = go->GetComponent(static_cast<ComponentType>(ct));
            if (comp) comp->deserializeJSON(cn["Data"]);
        }
    }

    info.m_overrides = savedOverrides;
    return true;
}

bool PrefabManager::createVariant(const fs::path& src, const fs::path& dst)
{
    if (src.empty() || dst.empty()) return false;

    Document doc;
    if (!PrefabSerializer::readDocument(src, doc)) return false;
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

    return PrefabSerializer::writeDocument(doc, dst);
}

GameObject* PrefabManager::spawnPrefab(const PrefabAsset& asset, Scene* scene)
{
    if (!scene || asset.getJSON().empty()) return nullptr;

    Document doc;
    doc.Parse(asset.getJSON().c_str());
    if (doc.HasParseError() || !doc.HasMember("GameObject") || !doc["GameObject"].IsObject())
    {
        DEBUG_ERROR("[ModuleAssets] Malformed JSON in prefab asset '%s'.", std::to_string(asset.getUID()).c_str());
        return nullptr;
    }

    GameObject* go = PrefabSerializer::deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    const auto& data = asset.getData();
    PrefabInstanceInfo& info = go->GetPrefabInfo();
    info.m_sourcePath = data.m_sourcePath;
    info.m_assetUID = data.m_assetUID;

    return go;
}

GameObject* PrefabManager::spawnPrefab(const fs::path& sourcePath, Scene* scene)
{
    if (!scene || sourcePath.empty()) return nullptr;

    auto asset = m_moduleAssets->loadAtPath<PrefabAsset>(sourcePath);
    if (asset) return spawnPrefab(*asset, scene);

    Document doc;
    if (!PrefabSerializer::loadDocument(sourcePath, doc)) return nullptr;

    GameObject* go = PrefabSerializer::deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    go->GetPrefabInfo().m_sourcePath = sourcePath;
    return go;
}
