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
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <FileIO.h>
#include <filesystem>
#include <string>

using namespace rapidjson;
namespace fs = std::filesystem;

namespace {

    PrefabInstanceComponent* getOrCreatePrefabComponent(GameObject* go)
    {
        auto* comp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
        if (!comp)
            comp = static_cast<PrefabInstanceComponent*>(go->AddComponentWithUID(ComponentType::PREFAB_INSTANCE, GenerateUID()));
        return comp;
    }

    void regeneratePrefabInstanceUIDs(GameObject* gameObject)
    {
        if (gameObject == nullptr)
        {
            return;
        }

        gameObject->SetUID(GenerateUID());

        Transform* transform = gameObject->GetTransform();

        if (transform != nullptr)
        {
            transform->setUID(GenerateUID());
        }

        for (Component* component : gameObject->GetAllComponents())
        {
            if (component == nullptr)
            {
                continue;
            }

            if (component->getType() == ComponentType::TRANSFORM)
            {
                continue;
            }

            component->setUID(GenerateUID());
        }

        if (transform == nullptr)
        {
            return;
        }

        for (GameObject* child : transform->getAllChildren())
        {
            regeneratePrefabInstanceUIDs(child);
        }
    }

} // anonymous namespace

PrefabManager::PrefabManager(ModuleAssets* moduleAssets) : m_moduleAssets(moduleAssets)
{
}

PrefabManager::~PrefabManager() = default;

bool PrefabManager::applyPrefab(const GameObject* go)
{
    auto* preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (!preComp || !preComp->isInstance()) return false;

    const fs::path& prefabPath = preComp->getData().m_sourcePath;

    const UID existingUID = m_moduleAssets->getIndex().findUID(prefabPath);
    AssetId ref;
    Prefab tempPrefab(ref);
    tempPrefab.setUID(isValidUID(existingUID) ? existingUID : GenerateUID());
    tempPrefab.buildFrom(const_cast<GameObject*>(go));
    tempPrefab.m_sourcePath = prefabPath;

    if (!m_moduleAssets->save(tempPrefab, prefabPath))
        return false;

    auto* goPreComp = const_cast<GameObject*>(go)->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (goPreComp)
    {
        goPreComp->getData().m_sourcePath = prefabPath;
        const UID assetUID = m_moduleAssets->getIndex().findUID(prefabPath);
        if (isValidUID(assetUID))
            goPreComp->getData().m_assetUID = assetUID;
    }

    if (isValidUID(existingUID))
        m_moduleAssets->unload(AssetId(existingUID));

    Scene* scene = app->getModuleScene()->getScene();
    if (!scene) return true;

    for (GameObject* instance : scene->getAllGameObjects())
    {
        if (!instance || instance == go) continue;
        auto* instComp = instance->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
        if (instComp && instComp->getData().m_sourcePath == prefabPath)
            revertPrefab(instance, scene);
    }

    scene->FixReferences();
    scene->markDirty();
    return true;
}

bool PrefabManager::revertPrefab(GameObject* go, Scene* scene)
{
    auto* preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (!preComp || !preComp->isInstance()) return false;

    const PrefabInstanceInfo& info = preComp->getData();

    auto raw = FileIO::read(info.m_sourcePath);
    Document doc;
    doc.Parse(reinterpret_cast<const char*>(raw.data()));
    if (raw.empty() || doc.HasParseError() || !doc.HasMember("GameObject"))
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

        auto readMember = [&](const char* lower, const char* upper) -> const Value*
            {
                if (tfNode.HasMember(upper)) return &tfNode[upper];
                if (tfNode.HasMember(lower)) return &tfNode[lower];
                return nullptr;
            };
        if (!isOverridden("position"))
        {
            if (const Value* v = readMember("position", "Position"))
            {
                if (v->IsArray() && v->Size() >= 3)
                    tf->setPosition(Vector3((*v)[0].GetFloat(), (*v)[1].GetFloat(), (*v)[2].GetFloat()));
            }
        }
        if (!isOverridden("rotation"))
        {
            if (const Value* v = readMember("rotation", "Rotation"))
            {
                if (v->IsArray() && v->Size() >= 4)
                    tf->setRotation(Quaternion((*v)[0].GetFloat(), (*v)[1].GetFloat(), (*v)[2].GetFloat(), (*v)[3].GetFloat()));
            }
        }
        if (!isOverridden("scale"))
        {
            if (const Value* v = readMember("scale", "Scale"))
            {
                if (v->IsArray() && v->Size() >= 3)
                    tf->setScale(Vector3((*v)[0].GetFloat(), (*v)[1].GetFloat(), (*v)[2].GetFloat()));
            }
        }
        tf->markDirty();
    }

    {
        uint32_t componentCount = goNode.HasMember("ComponentCount") ? goNode["ComponentCount"].GetUint() : 0;
        std::vector<ComponentType> fileTypes;
        fileTypes.reserve(componentCount);

        for (uint32_t i = 0; i < componentCount; ++i)
        {
            std::string key = "Component_" + std::to_string(i);
            if (!goNode.HasMember(key.c_str())) continue;

            const Value& cn = goNode[key.c_str()];
            if (!cn.HasMember("ComponentType")) continue;

            ComponentType ct = static_cast<ComponentType>(cn["ComponentType"].GetInt());
            fileTypes.push_back(ct);

            auto oit = savedOverrides.m_modifiedProperties.find(static_cast<int>(ct));
            if (oit != savedOverrides.m_modifiedProperties.end()
                && oit->second.count("properties") > 0)
                continue;

            Component* comp = go->GetComponent(ct);
            if (!comp && ct != ComponentType::PREFAB_INSTANCE && ct != ComponentType::TRANSFORM)
                comp = go->AddComponentWithUID(ct, GenerateUID());

            if (comp)
            {
                JsonArchive compArchive(ArchiveMode::Input);
                compArchive.setValue(cn);
                comp->serialize(compArchive);
            }
        }

        // Remove components that exist on the GO but are no longer in the prefab file
        // (skipping Transform, PrefabInstance, and explicitly added overrides)
        std::vector<Component*> toRemove;
        for (Component* comp : go->GetAllComponents())
        {
            ComponentType ct = comp->getType();
            if (ct == ComponentType::TRANSFORM || ct == ComponentType::PREFAB_INSTANCE)
                continue;
            if (std::find(fileTypes.begin(), fileTypes.end(), ct) == fileTypes.end())
            {
                auto oit = savedOverrides.m_modifiedProperties.find(static_cast<int>(ct));
                if (oit == savedOverrides.m_modifiedProperties.end())
                    toRemove.push_back(comp);
            }
        }
        for (Component* comp : toRemove)
            go->RemoveComponent(comp);
    }

    preComp->getData().m_overrides = savedOverrides;
    return true;
}

bool PrefabManager::createVariant(const fs::path& src, const fs::path& dst)
{
    if (src.empty() || dst.empty()) return false;

    auto raw = FileIO::read(src);
    Document doc;
    doc.Parse(reinterpret_cast<const char*>(raw.data()));
    if (raw.empty() || doc.HasParseError()) return false;

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

    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);

    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);
    if (ec) return false;

    FILE* file = fopen(dst.string().c_str(), "wb");
    if (!file) return false;
    fwrite(sb.GetString(), 1, sb.GetSize(), file);
    fclose(file);
    return true;
}

GameObject* PrefabManager::spawnPrefab(const Prefab& prefab, Scene* scene)
{
    auto clone = prefab.spawnClone();
    if (!clone) return nullptr;

    regeneratePrefabInstanceUIDs(clone.get());

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

GameObject* PrefabManager::spawnPrefab(const AssetId& ref, Scene* scene)
{
    if (!scene) return nullptr;

    AssetId mutableRef = ref;
    auto prefab = m_moduleAssets->load<Prefab>(mutableRef);
    if (!prefab) return nullptr;

    return spawnPrefab(*prefab, scene);
}
