#include "Globals.h"
#include "PrefabManager.h"

#include "Application.h"
#include "Component.h"
#include "ComponentType.h"
#include "ModuleFileSystem.h"
#include "GameObject.h"
#include "ModuleScene.h"
#include "Transform.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <functional>

using namespace rapidjson;
namespace fs = std::filesystem;

static constexpr int  PREFAB_FORMAT_VERSION = 2;
static constexpr char PREFAB_DIR[] = "Assets/Prefabs/";
static constexpr char PREFAB_EXT[] = ".prefab";

std::unordered_map<const GameObject*, PrefabInstanceData>& PrefabManager::registry()
{
    static std::unordered_map<const GameObject*, PrefabInstanceData> s_registry;
    return s_registry;
}

std::string PrefabManager::getPrefabPath(const std::string& name)
{
    return std::string(PREFAB_DIR) + name + PREFAB_EXT;
}

uint32_t PrefabManager::makePrefabUID(const std::string& name)
{
    uint32_t hash = 2166136261u;
    for (unsigned char character : name)
    {
        hash ^= character;
        hash *= 16777619u;
    }
    return hash ? hash : 1u;
}

bool PrefabManager::writePrefabDocument(Document& doc, const std::string& path)
{
    fs::create_directories(fs::path(path).parent_path());

    FILE* file = fopen(path.c_str(), "wb");
    if (!file)
    {
        return false;
    }

    char writeBuffer[65536];
    FileWriteStream outputStream(file, writeBuffer, sizeof(writeBuffer));
    PrettyWriter<FileWriteStream> writer(outputStream);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);
    fclose(file);
    return true;
}

bool PrefabManager::readPrefabDocument(const std::string& path, Document& doc)
{
    FILE* file = fopen(path.c_str(), "rb");
    if (!file)
    {
        return false;
    }

    char readBuffer[65536];
    FileReadStream inputStream(file, readBuffer, sizeof(readBuffer));
    doc.ParseStream(inputStream);
    fclose(file);
    return !doc.HasParseError();
}

static void serialiseTransform(const GameObject* go, Value& out, Document::AllocatorType& allocator)
{
    const Transform* transform = go->GetTransform();
    const Vector3& position = transform->getPosition();
    const Quaternion rotation = transform->getRotation();
    const Vector3& scale = transform->getScale();

    Value positionArray(kArrayType);
    positionArray.PushBack(position.x, allocator).PushBack(position.y, allocator).PushBack(position.z, allocator);

    Value rotationArray(kArrayType);
    rotationArray.PushBack(rotation.x, allocator).PushBack(rotation.y, allocator)
        .PushBack(rotation.z, allocator).PushBack(rotation.w, allocator);

    Value scaleArray(kArrayType);
    scaleArray.PushBack(scale.x, allocator).PushBack(scale.y, allocator).PushBack(scale.z, allocator);

    Value transformNode(kObjectType);
    transformNode.AddMember("position", positionArray, allocator);
    transformNode.AddMember("rotation", rotationArray, allocator);
    transformNode.AddMember("scale", scaleArray, allocator);

    out.AddMember("Transform", transformNode, allocator);
}

static void serialiseComponents(const GameObject* go, Value& out, Document::AllocatorType& allocator)
{
    Value componentArray(kArrayType);
    Document helperDoc;
    helperDoc.SetObject();

    for (Component* component : go->GetAllComponents())
    {
        if (component->getType() == ComponentType::TRANSFORM)
        {
            continue;
        }

        Value componentData = component->getJSON(helperDoc);
        Value componentDataCopy;
        componentDataCopy.CopyFrom(componentData, allocator);

        Value componentNode(kObjectType);
        componentNode.AddMember("Type", static_cast<int>(component->getType()), allocator);
        componentNode.AddMember("Data", componentDataCopy, allocator);

        componentArray.PushBack(componentNode, allocator);
    }

    out.AddMember("Components", componentArray, allocator);
}

static void serialiseNodeInto(const GameObject* go, Value& out, Document::AllocatorType& allocator)
{
    out.SetObject();
    out.AddMember("Name", Value(go->GetName().c_str(), allocator), allocator);
    out.AddMember("Active", go->GetActive(), allocator);

    const PrefabInstanceData* instanceData = PrefabManager::getInstanceData(go);
    if (instanceData)
    {
        Value prefabLink(kObjectType);
        prefabLink.AddMember("PrefabName", Value(instanceData->m_prefabName.c_str(), allocator), allocator);
        prefabLink.AddMember("PrefabUID", instanceData->m_prefabUID, allocator);
        out.AddMember("PrefabLink", prefabLink, allocator);
    }

    serialiseTransform(go, out, allocator);
    serialiseComponents(go, out, allocator);

    Value childrenArray(kArrayType);
    for (GameObject* child : go->GetTransform()->getAllChildren())
    {
        Value childNode;
        serialiseNodeInto(child, childNode, allocator);
        childrenArray.PushBack(childNode, allocator);
    }
    out.AddMember("Children", childrenArray, allocator);
}

static void deserialiseTransform(const Value& node, GameObject* go)
{
    if (!node.HasMember("Transform") || !node["Transform"].IsObject())
    {
        return;
    }

    Transform* transform = go->GetTransform();
    const Value& transformNode = node["Transform"];

    if (transformNode.HasMember("position") && transformNode["position"].IsArray())
    {
        const auto& position = transformNode["position"];
        transform->setPosition(Vector3(position[0].GetFloat(), position[1].GetFloat(), position[2].GetFloat()));
    }
    if (transformNode.HasMember("rotation") && transformNode["rotation"].IsArray())
    {
        const auto& rotation = transformNode["rotation"];
        transform->setRotation(Quaternion(rotation[0].GetFloat(), rotation[1].GetFloat(),
            rotation[2].GetFloat(), rotation[3].GetFloat()));
    }
    if (transformNode.HasMember("scale") && transformNode["scale"].IsArray())
    {
        const auto& scale = transformNode["scale"];
        transform->setScale(Vector3(scale[0].GetFloat(), scale[1].GetFloat(), scale[2].GetFloat()));
    }
}

static void deserialiseComponents(const Value& node, GameObject* go)
{
    if (!node.HasMember("Components") || !node["Components"].IsArray())
    {
        return;
    }

    for (SizeType index = 0; index < node["Components"].Size(); ++index)
    {
        const Value& componentNode = node["Components"][index];
        auto         componentType = static_cast<ComponentType>(componentNode["Type"].GetInt());
        Component* component = go->AddComponentWithUID(componentType, GenerateUID());

        if (component && componentNode.HasMember("Data") && componentNode["Data"].IsObject())
        {
            component->deserializeJSON(componentNode["Data"]);
        }
    }
}

GameObject* PrefabManager::deserialiseNode(const Value& node, ModuleScene* scene, GameObject* parent)
{
    if (!node.IsObject())
    {
        return nullptr;
    }

    const char* name = node.HasMember("Name") ? node["Name"].GetString() : "Unnamed";
    GameObject* gameObject = scene->createGameObjectWithUID(GenerateUID(), GenerateUID());

    if (!gameObject)
    {
        return nullptr;
    }

    gameObject->SetName(name);
    gameObject->SetActive(node.HasMember("Active") ? node["Active"].GetBool() : true);

    if (parent)
    {
        Transform* parentTransform = parent->GetTransform();
        Transform* childTransform = gameObject->GetTransform();
        childTransform->setRoot(parentTransform);
        parentTransform->addChild(gameObject);
        scene->removeFromRootList(gameObject);
    }

    if (node.HasMember("PrefabLink") && node["PrefabLink"].IsObject())
    {
        const Value& prefabLink = node["PrefabLink"];
        PrefabInstanceData linkData;
        linkData.m_prefabName = prefabLink.HasMember("PrefabName") ? prefabLink["PrefabName"].GetString() : "";
        linkData.m_prefabUID = prefabLink.HasMember("PrefabUID") ? prefabLink["PrefabUID"].GetUint() : 0;
        if (!linkData.m_prefabName.empty())
        {
            linkInstance(gameObject, linkData);
        }
    }

    deserialiseTransform(node, gameObject);
    deserialiseComponents(node, gameObject);

    if (node.HasMember("Children") && node["Children"].IsArray())
    {
        for (SizeType index = 0; index < node["Children"].Size(); ++index)
        {
            deserialiseNode(node["Children"][index], scene, gameObject);
        }
    }

    return gameObject;
}

std::string PrefabManager::serializeGameObject(const GameObject* go)
{
    if (!go)
    {
        return {};
    }

    Document doc;
    doc.SetObject();

    Value gameObjectNode;
    serialiseNodeInto(go, gameObjectNode, doc.GetAllocator());
    doc.AddMember("GameObject", gameObjectNode, doc.GetAllocator());

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

GameObject* PrefabManager::deserializeGameObject(const std::string& data, ModuleScene* scene)
{
    if (data.empty() || !scene)
    {
        return nullptr;
    }

    Document doc;
    doc.Parse(data.c_str());

    if (doc.HasParseError() || !doc.HasMember("GameObject") || !doc["GameObject"].IsObject())
    {
        return nullptr;
    }

    return deserialiseNode(doc["GameObject"], scene, nullptr);
}

bool PrefabManager::isPrefabInstance(const GameObject* go)
{
    return go && registry().count(go) > 0;
}

std::string PrefabManager::getPrefabName(const GameObject* go)
{
    const PrefabInstanceData* data = getInstanceData(go);
    return data ? data->m_prefabName : "";
}

uint32_t PrefabManager::getPrefabUID(const GameObject* go)
{
    const PrefabInstanceData* data = getInstanceData(go);
    return data ? data->m_prefabUID : 0;
}

const PrefabInstanceData* PrefabManager::getInstanceData(const GameObject* go)
{
    auto it = registry().find(go);
    return (it != registry().end()) ? &it->second : nullptr;
}

PrefabInstanceData* PrefabManager::getInstanceDataMutable(GameObject* go)
{
    auto it = registry().find(go);
    return (it != registry().end()) ? &it->second : nullptr;
}

void PrefabManager::linkInstance(GameObject* go, const PrefabInstanceData& data)
{
    if (go)
    {
        registry()[go] = data;
    }
}

void PrefabManager::unlinkInstance(GameObject* go)
{
    if (go)
    {
        registry().erase(go);
    }
}

void PrefabManager::markPropertyOverride(GameObject* go, int componentType, const std::string& propertyName)
{
    PrefabInstanceData* instance = getInstanceDataMutable(go);
    if (instance)
    {
        instance->m_overrides.m_modifiedProperties[componentType].insert(propertyName);
    }
}

void PrefabManager::clearComponentOverrides(GameObject* go, int componentType)
{
    PrefabInstanceData* instance = getInstanceDataMutable(go);
    if (!instance)
    {
        return;
    }

    instance->m_overrides.m_modifiedProperties.erase(componentType);

    auto eraseType = [componentType](std::vector<int>& vec)
        {
            vec.erase(std::remove(vec.begin(), vec.end(), componentType), vec.end());
        };
    eraseType(instance->m_overrides.m_addedComponentTypes);
    eraseType(instance->m_overrides.m_removedComponentTypes);
}

void PrefabManager::clearAllOverrides(GameObject* go)
{
    PrefabInstanceData* instance = getInstanceDataMutable(go);
    if (instance)
    {
        instance->m_overrides.clear();
    }
}

void PrefabManager::markComponentAdded(GameObject* go, int componentType)
{
    PrefabInstanceData* instance = getInstanceDataMutable(go);
    if (!instance)
    {
        return;
    }

    auto& added = instance->m_overrides.m_addedComponentTypes;
    if (std::find(added.begin(), added.end(), componentType) == added.end())
    {
        added.push_back(componentType);
    }

    auto& removed = instance->m_overrides.m_removedComponentTypes;
    removed.erase(std::remove(removed.begin(), removed.end(), componentType), removed.end());
}

void PrefabManager::markComponentRemoved(GameObject* go, int componentType)
{
    PrefabInstanceData* instance = getInstanceDataMutable(go);
    if (!instance)
    {
        return;
    }

    auto& removed = instance->m_overrides.m_removedComponentTypes;
    if (std::find(removed.begin(), removed.end(), componentType) == removed.end())
    {
        removed.push_back(componentType);
    }

    auto& added = instance->m_overrides.m_addedComponentTypes;
    added.erase(std::remove(added.begin(), added.end(), componentType), added.end());

    instance->m_overrides.m_modifiedProperties.erase(componentType);
}

bool PrefabManager::createPrefab(const GameObject* go, const std::string& prefabName)
{
    if (!go || prefabName.empty())
    {
        return false;
    }

    std::error_code errorCode;
    fs::create_directories(PREFAB_DIR, errorCode);

    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("PrefabName", Value(prefabName.c_str(), allocator), allocator);
    doc.AddMember("Version", PREFAB_FORMAT_VERSION, allocator);
    doc.AddMember("PrefabUID", makePrefabUID(prefabName), allocator);

    const PrefabInstanceData* instanceData = getInstanceData(go);
    if (instanceData && !instanceData->m_prefabName.empty() && instanceData->m_prefabName != prefabName)
    {
        doc.AddMember("VariantOf", Value(instanceData->m_prefabName.c_str(), allocator), allocator);
    }

    Value gameObjectNode;
    serialiseNodeInto(go, gameObjectNode, allocator);
    doc.AddMember("GameObject", gameObjectNode, allocator);

    if (!writePrefabDocument(doc, getPrefabPath(prefabName)))
    {
        return false;
    }

    app->getModuleFileSystem()->rebuild();
    return true;
}

GameObject* PrefabManager::instantiatePrefab(const std::string& prefabName, ModuleScene* scene)
{
    if (!scene || prefabName.empty())
    {
        return nullptr;
    }

    std::string prefabPath = getPrefabPath(prefabName);
    if (!fs::exists(prefabPath))
    {
        return nullptr;
    }

    Document doc;
    if (!readPrefabDocument(prefabPath, doc) || !doc.HasMember("GameObject") || !doc["GameObject"].IsObject())
    {
        return nullptr;
    }

    GameObject* gameObject = deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!gameObject)
    {
        return nullptr;
    }

    PrefabInstanceData instanceData;
    instanceData.m_prefabName = prefabName;
    instanceData.m_prefabUID = doc.HasMember("PrefabUID") ? doc["PrefabUID"].GetUint() : makePrefabUID(prefabName);
    linkInstance(gameObject, instanceData);

    return gameObject;
}

bool PrefabManager::applyToPrefab(const GameObject* go, bool respectOverrides)
{
    const PrefabInstanceData* instance = getInstanceData(go);
    if (!instance || instance->m_prefabName.empty())
    {
        return false;
    }

    if (!respectOverrides)
    {
        return createPrefab(go, instance->m_prefabName);
    }

    fs::create_directories(PREFAB_DIR);

    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("PrefabName", Value(instance->m_prefabName.c_str(), allocator), allocator);
    doc.AddMember("Version", PREFAB_FORMAT_VERSION, allocator);
    doc.AddMember("PrefabUID", instance->m_prefabUID, allocator);

    if (!instance->m_overrides.isEmpty())
    {
        Value overrideMap(kObjectType);

        Value modifiedProperties(kObjectType);
        for (const auto& [componentType, properties] : instance->m_overrides.m_modifiedProperties)
        {
            Value propertyArray(kArrayType);
            for (const auto& propertyName : properties)
            {
                propertyArray.PushBack(Value(propertyName.c_str(), allocator), allocator);
            }
            std::string key = std::to_string(componentType);
            modifiedProperties.AddMember(Value(key.c_str(), allocator), propertyArray, allocator);
        }
        overrideMap.AddMember("ModifiedProperties", modifiedProperties, allocator);

        Value addedArray(kArrayType);
        for (int componentType : instance->m_overrides.m_addedComponentTypes)
        {
            addedArray.PushBack(componentType, allocator);
        }
        overrideMap.AddMember("AddedComponents", addedArray, allocator);

        Value removedArray(kArrayType);
        for (int componentType : instance->m_overrides.m_removedComponentTypes)
        {
            removedArray.PushBack(componentType, allocator);
        }
        overrideMap.AddMember("RemovedComponents", removedArray, allocator);

        doc.AddMember("OverrideMap", overrideMap, allocator);
    }

    Value gameObjectNode;
    serialiseNodeInto(go, gameObjectNode, allocator);
    doc.AddMember("GameObject", gameObjectNode, allocator);

    return writePrefabDocument(doc, getPrefabPath(instance->m_prefabName));
}

bool PrefabManager::revertToPrefab(GameObject* go, ModuleScene* scene)
{
    PrefabInstanceData* instance = getInstanceDataMutable(go);
    if (!instance || instance->m_prefabName.empty())
        return false;

    std::string prefabPath = getPrefabPath(instance->m_prefabName);
    if (!fs::exists(prefabPath))
        return false;

    Document doc;
    if (!readPrefabDocument(prefabPath, doc) || !doc.HasMember("GameObject"))
        return false;

    const Value& gameObjectNode = doc["GameObject"];
    PrefabOverrideRecord savedOverrides = instance->m_overrides;
    const int transformType = static_cast<int>(ComponentType::TRANSFORM);

    if (gameObjectNode.HasMember("Transform") && gameObjectNode["Transform"].IsObject())
    {
        Transform* transform = go->GetTransform();
        const Value& transformNode = gameObjectNode["Transform"];

        auto overrideIterator = savedOverrides.m_modifiedProperties.find(transformType);
        const std::unordered_set<std::string>* overrideSet =
            (overrideIterator != savedOverrides.m_modifiedProperties.end()) ? &overrideIterator->second : nullptr;

        auto isOverridden = [&](const char* propertyName)
            {
                return overrideSet && overrideSet->count(propertyName) > 0;
            };

        if (!isOverridden("position") && transformNode.HasMember("position") && transformNode["position"].IsArray())
        {
            const auto& position = transformNode["position"];
            transform->setPosition(Vector3(position[0].GetFloat(), position[1].GetFloat(), position[2].GetFloat()));
        }
        if (!isOverridden("rotation") && transformNode.HasMember("rotation") && transformNode["rotation"].IsArray())
        {
            const auto& rotation = transformNode["rotation"];
            transform->setRotation(Quaternion(rotation[0].GetFloat(), rotation[1].GetFloat(),
                rotation[2].GetFloat(), rotation[3].GetFloat()));
        }
        if (!isOverridden("scale") && transformNode.HasMember("scale") && transformNode["scale"].IsArray())
        {
            const auto& scale = transformNode["scale"];
            transform->setScale(Vector3(scale[0].GetFloat(), scale[1].GetFloat(), scale[2].GetFloat()));
        }
        transform->markDirty();
    }

    if (gameObjectNode.HasMember("Components") && gameObjectNode["Components"].IsArray())
    {
        const Value& componentsArray = gameObjectNode["Components"];
        for (SizeType i = 0; i < componentsArray.Size(); ++i)
        {
            const Value& componentNode = componentsArray[i];
            if (!componentNode.HasMember("Type") || !componentNode.HasMember("Data"))
                continue;

            const int componentType = componentNode["Type"].GetInt();

            auto overrideIt = savedOverrides.m_modifiedProperties.find(componentType);
            if (overrideIt != savedOverrides.m_modifiedProperties.end()
                && overrideIt->second.count("properties") > 0)
                continue;

            Component* component = go->GetComponent(static_cast<ComponentType>(componentType));
            if (component)
            {
                component->deserializeJSON(componentNode["Data"]);
            }
        }
    }

    instance->m_overrides = savedOverrides;
    return true;
}

bool PrefabManager::createVariant(const std::string& sourcePrefabName, const std::string& destinationPrefabName)
{
    if (sourcePrefabName.empty() || destinationPrefabName.empty())
    {
        return false;
    }

    std::string sourcePath = getPrefabPath(sourcePrefabName);
    if (!fs::exists(sourcePath))
    {
        return false;
    }

    fs::create_directories(PREFAB_DIR);

    Document doc;
    if (!readPrefabDocument(sourcePath, doc))
    {
        return false;
    }

    auto& allocator = doc.GetAllocator();

    auto setOrAdd = [&](const char* key, const std::string& value)
        {
            if (doc.HasMember(key))
            {
                doc[key].SetString(value.c_str(), allocator);
            }
            else
            {
                doc.AddMember(Value(key, allocator), Value(value.c_str(), allocator), allocator);
            }
        };

    setOrAdd("PrefabName", destinationPrefabName);
    setOrAdd("VariantOf", sourcePrefabName);

    const uint32_t destinationUID = makePrefabUID(destinationPrefabName);
    if (doc.HasMember("PrefabUID"))
    {
        doc["PrefabUID"].SetUint(destinationUID);
    }
    else
    {
        doc.AddMember("PrefabUID", destinationUID, allocator);
    }

    return writePrefabDocument(doc, getPrefabPath(destinationPrefabName));
}

std::vector<std::string> PrefabManager::listPrefabs()
{
    std::vector<std::string> names;
    if (!fs::exists(PREFAB_DIR))
    {
        return names;
    }

    std::error_code errorCode;
    fs::directory_iterator iterator(PREFAB_DIR, errorCode);
    if (errorCode)
    {
        return names;
    }

    for (const auto& entry : iterator)
    {
        if (entry.is_regular_file() && entry.path().extension() == PREFAB_EXT)
        {
            names.push_back(entry.path().stem().string());
        }
    }

    return names;
}

std::vector<PrefabManager::PrefabInfo> PrefabManager::listPrefabsInfo()
{
    std::vector<PrefabInfo> results;
    if (!fs::exists(PREFAB_DIR))
    {
        return results;
    }

    auto countChildren = [](auto& self, const Value& node) -> int
        {
            int count = 0;
            if (node.HasMember("Children") && node["Children"].IsArray())
            {
                count += static_cast<int>(node["Children"].Size());
                for (SizeType index = 0; index < node["Children"].Size(); ++index)
                {
                    count += self(self, node["Children"][index]);
                }
            }
            return count;
        };

    std::error_code errorCode;
    fs::directory_iterator iterator(PREFAB_DIR, errorCode);
    if (errorCode)
    {
        return results;
    }

    for (const auto& entry : iterator)
    {
        if (!entry.is_regular_file() || entry.path().extension() != PREFAB_EXT)
        {
            continue;
        }

        Document doc;
        if (!readPrefabDocument(entry.path().string(), doc))
        {
            continue;
        }

        PrefabInfo info;
        info.m_name = entry.path().stem().string();
        info.m_uid = doc.HasMember("PrefabUID") ? doc["PrefabUID"].GetUint() : 0;
        info.m_version = doc.HasMember("Version") ? doc["Version"].GetInt() : 0;

        if (doc.HasMember("VariantOf") && doc["VariantOf"].IsString())
        {
            info.m_variantOf = doc["VariantOf"].GetString();
            info.m_isVariant = true;
        }

        if (doc.HasMember("GameObject") && doc["GameObject"].IsObject())
        {
            const Value& gameObjectNode = doc["GameObject"];

            std::vector<std::string> componentNames = { "Transform" };
            if (gameObjectNode.HasMember("Components") && gameObjectNode["Components"].IsArray())
            {
                for (SizeType index = 0; index < gameObjectNode["Components"].Size(); ++index)
                {
                    auto type = static_cast<ComponentType>(gameObjectNode["Components"][index]["Type"].GetInt());
                    componentNames.push_back(ComponentTypeToString(type));
                }
            }

            for (size_t index = 0; index < componentNames.size(); ++index)
            {
                if (index)
                {
                    info.m_componentSummary += ", ";
                }
                info.m_componentSummary += componentNames[index];
            }

            info.m_childCount = countChildren(countChildren, gameObjectNode);
        }

        results.push_back(std::move(info));
    }

    return results;
}

bool PrefabManager::prefabExists(const std::string& prefabName)
{
    return fs::exists(getPrefabPath(prefabName));
}