#include "Globals.h"
#include "PrefabManager.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "MD5.h"
#include "GameObject.h"
#include "ComponentType.h"
#include "Component.h"
#include "Transform.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include "PrefabAsset.h"

using namespace rapidjson;
namespace fs = std::filesystem;

static constexpr int         PREFAB_FORMAT_VERSION = 2;
static constexpr const char* PREFAB_EXT = ".prefab";


std::unordered_map<const GameObject*, PrefabData>& PrefabManager::registry()
{
    static std::unordered_map<const GameObject*, PrefabData> s_registry;
    return s_registry;
}


bool PrefabManager::writePrefabDocument(Document& doc, const fs::path& path)
{
    fs::create_directories(path.parent_path());
    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file)
    {
        return false;
    }
    char buf[65536];

    FileWriteStream os(file, buf, sizeof(buf));
    PrettyWriter<FileWriteStream> writer(os);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);
    fclose(file);
    return true;
}

bool PrefabManager::readPrefabDocument(const fs::path& path, Document& doc)
{
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file)
    {
        return false;
    }
    char buf[65536];
    FileReadStream is(file, buf, sizeof(buf));
    doc.ParseStream(is);
    fclose(file);
    return !doc.HasParseError();
}


static void serialiseTransform(const GameObject* go, Value& out,
    Document::AllocatorType& alloc)
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

static void serialiseComponents(const GameObject* go, Value& out,
    Document::AllocatorType& alloc)
{
    Value compArray(kArrayType);
    Document helperDoc;
    helperDoc.SetObject();

    for (Component* comp : go->GetAllComponents())
    {
        if (comp->getType() == ComponentType::TRANSFORM) continue;

        Value data = comp->getJSON(helperDoc);
        Value dataCopy;
        dataCopy.CopyFrom(data, alloc);

        Value compNode(kObjectType);
        compNode.AddMember("Type", static_cast<int>(comp->getType()), alloc);
        compNode.AddMember("Data", dataCopy, alloc);
        compArray.PushBack(compNode, alloc);
    }

    out.AddMember("Components", compArray, alloc);
}

static void serialiseNodeInto(const GameObject* go, Value& out,
    Document::AllocatorType& alloc)
{
    out.SetObject();
    out.AddMember("Name", Value(go->GetName().c_str(), alloc), alloc);
    out.AddMember("Active", go->GetActive(), alloc);


    const PrefabData* data = PrefabManager::getInstanceData(go);
    if (data)
    {
        Value prefabLink(kObjectType);
        prefabLink.AddMember("SourcePath", Value(data->m_sourcePath.string().c_str(), alloc), alloc);
        prefabLink.AddMember("AssetUID", Value(data->m_assetUID.c_str(), alloc), alloc);
        prefabLink.AddMember("PrefabUID", static_cast<uint64_t>(data->m_prefabUID), alloc);
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


static void deserialiseTransform(const Value& node, GameObject* go)
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
        tf->setRotation(Quaternion(r[0].GetFloat(), r[1].GetFloat(),
            r[2].GetFloat(), r[3].GetFloat()));
    }
    if (tfNode.HasMember("scale") && tfNode["scale"].IsArray())
    {
        const auto& s = tfNode["scale"];
        tf->setScale(Vector3(s[0].GetFloat(), s[1].GetFloat(), s[2].GetFloat()));
    }
}

static void deserialiseComponents(const Value& node, GameObject* go)
{
    if (!node.HasMember("Components") || !node["Components"].IsArray()) return;
    for (SizeType i = 0; i < node["Components"].Size(); ++i)
    {
        const Value& cn = node["Components"][i];
        auto         type = static_cast<ComponentType>(cn["Type"].GetInt());
        Component* comp = go->AddComponentWithUID(type, GenerateUID());
        if (comp && cn.HasMember("Data") && cn["Data"].IsObject())
        {
            comp->deserializeJSON(cn["Data"]);
        }
    }
}

GameObject* PrefabManager::deserialiseNode(const Value& node, Scene* scene, GameObject* parent)
{
    if (!node.IsObject()) return nullptr;

    const char* name = node.HasMember("Name") ? node["Name"].GetString() : "Unnamed";
    GameObject* go = scene->createGameObjectWithUID(GenerateUID(), GenerateUID());
    if (!go) return nullptr;

    go->SetName(name);
    go->SetActive(node.HasMember("Active") ? node["Active"].GetBool() : true);

    if (parent)
    {
        Transform* parentTf = parent->GetTransform();
        Transform* childTf = go->GetTransform();
        childTf->setRoot(parentTf);
        parentTf->addChild(go);
        scene->removeFromRootList(go);
    }

    // Restore the prefab link using full source path + GO UID.
    if (node.HasMember("PrefabLink") && node["PrefabLink"].IsObject())
    {
        const Value& pl = node["PrefabLink"];
        PrefabData linkData;
        if (pl.HasMember("SourcePath") && pl["SourcePath"].IsString())
        {
            linkData.m_sourcePath = pl["SourcePath"].GetString();
        }

        if (pl.HasMember("AssetUID") && pl["AssetUID"].IsString())
        {
            linkData.m_assetUID = pl["AssetUID"].GetString();
        }

        if (pl.HasMember("PrefabUID") && pl["PrefabUID"].IsUint64())
        {
            linkData.m_prefabUID = static_cast<UID>(pl["PrefabUID"].GetUint64());
        }

        linkData.m_name = linkData.m_sourcePath.stem().string();

        if (!linkData.m_sourcePath.empty())
        {
            linkInstance(go, linkData);
        }
    }

    deserialiseTransform(node, go);
    deserialiseComponents(node, go);

    if (node.HasMember("Children") && node["Children"].IsArray())
    {
        for (SizeType i = 0; i < node["Children"].Size(); ++i)
        {
            deserialiseNode(node["Children"][i], scene, go);
        }
    }

    return go;
}

std::string PrefabManager::buildPrefabJSON(const GameObject* go, const fs::path& savePath)
{
    if (!go || savePath.empty()) return {};

    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    const std::string pathStr = savePath.string();
    const std::string name = savePath.stem().string();

    doc.AddMember("SourcePath", Value(pathStr.c_str(), alloc), alloc);
    doc.AddMember("Name", Value(name.c_str(), alloc), alloc);
    doc.AddMember("Version", PREFAB_FORMAT_VERSION, alloc);

    doc.AddMember("PrefabUID", static_cast<UID>(go->GetID()), alloc);

    // Record variant relationship when this GO is itself a prefab instance
    // of a different prefab.
    const PrefabData* existing = getInstanceData(go);
    if (existing && !existing->m_sourcePath.empty() && existing->m_sourcePath != savePath)
    {
        doc.AddMember("VariantOf", Value(existing->m_sourcePath.string().c_str(), alloc), alloc);
    }

    Value gameObjectNode;
    serialiseNodeInto(go, gameObjectNode, alloc);
    doc.AddMember("GameObject", gameObjectNode, alloc);

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}

std::string PrefabManager::serializeGameObject(const GameObject* go)
{
    if (!go) return {};
    Document doc;
    doc.SetObject();
    Value goNode;
    serialiseNodeInto(go, goNode, doc.GetAllocator());
    doc.AddMember("GameObject", goNode, doc.GetAllocator());
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}

GameObject* PrefabManager::deserializeGameObject(const std::string& data, Scene* scene)
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


GameObject* PrefabManager::instantiatePrefab(const PrefabAsset& asset, Scene* scene)
{
    if (!scene || asset.getJSON().empty()) return nullptr;

    Document doc;
    doc.Parse(asset.getJSON().c_str());
    if (doc.HasParseError() || !doc.HasMember("GameObject") || !doc["GameObject"].IsObject())
    {
        DEBUG_ERROR("[PrefabManager] Malformed JSON in asset '%s'.", asset.getId().c_str());
        return nullptr;
    }

    GameObject* go = deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;


    PrefabData instanceData = asset.getData();
    instanceData.m_json = {};
    linkInstance(go, instanceData);
    return go;
}

// Convenience overload: resolve by full path, asset system first.
GameObject* PrefabManager::instantiatePrefab(const fs::path& sourcePath, Scene* scene)
{
    if (!scene || sourcePath.empty()) return nullptr;

    auto asset = app->getModuleAssets()->loadAtPath<PrefabAsset>(sourcePath);
    if (asset) {
        return instantiatePrefab(*asset, scene);
    }

    if (!fs::exists(sourcePath)) 
    {
        return nullptr;
    }

    Document doc;
    if (!readPrefabDocument(sourcePath, doc) || !doc.HasMember("GameObject") || !doc["GameObject"].IsObject())
    {
        return nullptr;
    }


    GameObject* go = deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go)
    {
        return nullptr;
    }

    PrefabData instanceData;
    instanceData.m_sourcePath = sourcePath;
    instanceData.m_name = sourcePath.stem().string();
    instanceData.m_prefabUID = doc.HasMember("PrefabUID") && doc["PrefabUID"].IsUint64() ? static_cast<UID>(doc["PrefabUID"].GetUint64()) : 0;
    linkInstance(go, instanceData);
    return go;
}

bool PrefabManager::createPrefab(const GameObject* go, const fs::path& savePath)
{
    if (!go || savePath.empty()) return false;

    const std::string json = buildPrefabJSON(go, savePath);
    if (json.empty()) return false;

    Document doc;
    doc.Parse(json.c_str());
    if (!writePrefabDocument(doc, savePath)) return false;

    app->getModuleAssets()->refresh();
    return true;
}

bool PrefabManager::applyToPrefab(const GameObject* go, bool respectOverrides)
{
    const PrefabData* data = getInstanceData(go);
    if (!data || data->m_sourcePath.empty()) return false;
    if (!respectOverrides) return createPrefab(go, data->m_sourcePath);

    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    const std::string pathStr = data->m_sourcePath.string();
    doc.AddMember("SourcePath", Value(pathStr.c_str(), alloc), alloc);
    doc.AddMember("Name", Value(data->m_name.c_str(), alloc), alloc);
    doc.AddMember("Version", PREFAB_FORMAT_VERSION, alloc);
    doc.AddMember("PrefabUID", static_cast<uint64_t>(data->m_prefabUID), alloc);

    if (!data->m_overrides.isEmpty())
    {
        Value overrideMap(kObjectType);
        Value modProps(kObjectType);
        for (const auto& [ct, props] : data->m_overrides.m_modifiedProperties)
        {
            Value arr(kArrayType);
            for (const auto& p : props)
            {
                arr.PushBack(Value(p.c_str(), alloc), alloc);

            }
            const std::string key = std::to_string(ct);
            modProps.AddMember(Value(key.c_str(), alloc), arr, alloc);
        }
        overrideMap.AddMember("ModifiedProperties", modProps, alloc);

        Value addedArr(kArrayType);
        for (int t : data->m_overrides.m_addedComponentTypes)
        {
            addedArr.PushBack(t, alloc);
        }

        overrideMap.AddMember("AddedComponents", addedArr, alloc);

        Value removedArr(kArrayType);
        for (int t : data->m_overrides.m_removedComponentTypes)
        {
            removedArr.PushBack(t, alloc);
        }

        overrideMap.AddMember("RemovedComponents", removedArr, alloc);

        doc.AddMember("OverrideMap", overrideMap, alloc);
    }

    Value goNode;
    serialiseNodeInto(go, goNode, alloc);
    doc.AddMember("GameObject", goNode, alloc);

    return writePrefabDocument(doc, data->m_sourcePath);
}

bool PrefabManager::revertToPrefab(GameObject* go, Scene* scene)
{
    PrefabData* data = getInstanceDataMutable(go);
    if (!data || data->m_sourcePath.empty()) return false;

    Document doc;
    bool loaded = false;

    if (isValidAsset(data->m_assetUID))
    {
        auto asset = app->getModuleAssets()->load<PrefabAsset>(data->m_assetUID);
        if (asset && !asset->getJSON().empty())
        {
            doc.Parse(asset->getJSON().c_str());
            loaded = !doc.HasParseError() && doc.HasMember("GameObject");
        }
    }
    if (!loaded)
    {
        if (!fs::exists(data->m_sourcePath))
        {
            return false;
        }

        if (!readPrefabDocument(data->m_sourcePath, doc) || !doc.HasMember("GameObject")) 
        {
            return false;
        }
    }

    const Value& goNode = doc["GameObject"];
    PrefabOverrideRecord savedOverrides = data->m_overrides;
    const int            transformType = static_cast<int>(ComponentType::TRANSFORM);

    if (goNode.HasMember("Transform") && goNode["Transform"].IsObject())
    {
        Transform* tf = go->GetTransform();
        const Value& tfNode = goNode["Transform"];

        auto oit = savedOverrides.m_modifiedProperties.find(transformType);
        const std::unordered_set<std::string>* overrideSet =
            (oit != savedOverrides.m_modifiedProperties.end()) ? &oit->second : nullptr;

        auto isOverridden = [&](const char* prop)
            { 
                return overrideSet && overrideSet->count(prop) > 0; 
            };

        if (!isOverridden("position") && tfNode.HasMember("position") && tfNode["position"].IsArray())
        {
            const auto& p = tfNode["position"];
            tf->setPosition(Vector3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat()));
        }
        if (!isOverridden("rotation") && tfNode.HasMember("rotation") && tfNode["rotation"].IsArray())
        {
            const auto& r = tfNode["rotation"];
            tf->setRotation(Quaternion(r[0].GetFloat(), r[1].GetFloat(),
                r[2].GetFloat(), r[3].GetFloat()));
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
            if (oit != savedOverrides.m_modifiedProperties.end() && oit->second.count("properties") > 0) continue;
            Component* comp = go->GetComponent(static_cast<ComponentType>(ct));
            if (comp) comp->deserializeJSON(cn["Data"]);
        }
    }

    data->m_overrides = savedOverrides;
    return true;
}

bool PrefabManager::createVariant(const fs::path& sourcePath,
    const fs::path& destinationPath)
{
    if (sourcePath.empty() || destinationPath.empty()) return false;
    if (!fs::exists(sourcePath)) return false;

    Document doc;
    if (!readPrefabDocument(sourcePath, doc)) return false;
    auto& alloc = doc.GetAllocator();

    const std::string dstStr = destinationPath.string();
    const std::string dstName = destinationPath.stem().string();
    const std::string srcStr = sourcePath.string();

    auto setOrAdd = [&](const char* key, const std::string& value)
        {
            if (doc.HasMember(key))
            {
                doc[key].SetString(value.c_str(), alloc);
            }
            else
            {
                doc.AddMember(Value(key, alloc), Value(value.c_str(), alloc), alloc);
            }
        };

    setOrAdd("SourcePath", dstStr);
    setOrAdd("Name", dstName);
    setOrAdd("VariantOf", srcStr);


    return writePrefabDocument(doc, destinationPath);
}


bool PrefabManager::isPrefabInstance(const GameObject* go)
{
    return go && registry().count(go) > 0;
}

std::string PrefabManager::getPrefabName(const GameObject* go)
{
    const PrefabData* d = getInstanceData(go);
    return d ? d->m_name : "";
}

UID PrefabManager::getPrefabUID(const GameObject* go)
{
    const PrefabData* d = getInstanceData(go);
    return d ? d->m_prefabUID : 0;
}

const PrefabData* PrefabManager::getInstanceData(const GameObject* go)
{
    auto it = registry().find(go);
    return (it != registry().end()) ? &it->second : nullptr;
}

PrefabData* PrefabManager::getInstanceDataMutable(GameObject* go)
{
    auto it = registry().find(go);
    return (it != registry().end()) ? &it->second : nullptr;
}

void PrefabManager::linkInstance(GameObject* go, const PrefabData& data)
{
    if (go) registry()[go] = data;
}

void PrefabManager::unlinkInstance(GameObject* go)
{
    if (go) registry().erase(go);
}

void PrefabManager::markPropertyOverride(GameObject* go, int ct,
    const std::string& prop)
{
    PrefabData* d = getInstanceDataMutable(go);
    if (d) d->m_overrides.m_modifiedProperties[ct].insert(prop);
}

void PrefabManager::clearComponentOverrides(GameObject* go, int ct)
{
    PrefabData* d = getInstanceDataMutable(go);
    if (!d) return;
    d->m_overrides.m_modifiedProperties.erase(ct);
    auto erase = [ct](std::vector<int>& v)
        { 
            v.erase(std::remove(v.begin(), v.end(), ct), v.end()); 
        };
    erase(d->m_overrides.m_addedComponentTypes);
    erase(d->m_overrides.m_removedComponentTypes);
}

void PrefabManager::clearAllOverrides(GameObject* go)
{
    PrefabData* d = getInstanceDataMutable(go);
    if (d) {
        d->m_overrides.clear();
    }
}

void PrefabManager::markComponentAdded(GameObject* go, int ct)
{
    PrefabData* d = getInstanceDataMutable(go);
    if (!d) return;
    auto& added = d->m_overrides.m_addedComponentTypes;
    if (std::find(added.begin(), added.end(), ct) == added.end())
    {
        added.push_back(ct);
    }

    auto& removed = d->m_overrides.m_removedComponentTypes;
    removed.erase(std::remove(removed.begin(), removed.end(), ct), removed.end());
}

void PrefabManager::markComponentRemoved(GameObject* go, int ct)
{
    PrefabData* d = getInstanceDataMutable(go);
    if (!d) return;
    auto& removed = d->m_overrides.m_removedComponentTypes;
    if (std::find(removed.begin(), removed.end(), ct) == removed.end())
    {
        removed.push_back(ct);
    }

    auto& added = d->m_overrides.m_addedComponentTypes;
    added.erase(std::remove(added.begin(), added.end(), ct), added.end());
    d->m_overrides.m_modifiedProperties.erase(ct);
}


std::vector<fs::path> PrefabManager::listPrefabs(const fs::path& searchRoot)
{
    std::vector<fs::path> paths;
    if (!fs::exists(searchRoot)) return paths;
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(searchRoot, ec))
    {
        if (entry.is_regular_file() && entry.path().extension() == PREFAB_EXT)
        {
            paths.push_back(entry.path());
        }
    }

    return paths;
}

std::vector<PrefabManager::PrefabInfo> PrefabManager::listPrefabsInfo(
    const fs::path& searchRoot)
{
    std::vector<PrefabInfo> results;
    if (!fs::exists(searchRoot)) return results;

    auto countChildren = [](auto& self, const Value& node) -> int
        {
            int count = 0;
            if (node.HasMember("Children") && node["Children"].IsArray())
            {
                count += static_cast<int>(node["Children"].Size());
                for (SizeType i = 0; i < node["Children"].Size(); ++i)
                {
                    count += self(self, node["Children"][i]);
                }

            }
            return count;
        };

    std::error_code ec;
    for (const auto& entry :
        fs::recursive_directory_iterator(searchRoot, ec))
    {
        if (!entry.is_regular_file() || entry.path().extension() != PREFAB_EXT)
            continue;

        Document doc;
        if (!readPrefabDocument(entry.path(), doc)) continue;

        PrefabInfo info;
        info.m_sourcePath = entry.path();
        info.m_name = entry.path().stem().string();
        info.m_version = doc.HasMember("Version") ? doc["Version"].GetInt() : 0;

        // PrefabUID is now a uint64_t GO UID.
        if (doc.HasMember("PrefabUID") && doc["PrefabUID"].IsUint64())
        {
            info.m_uid = static_cast<UID>(doc["PrefabUID"].GetUint64());
        }


        if (doc.HasMember("VariantOf") && doc["VariantOf"].IsString())
        {
            info.m_variantOf = doc["VariantOf"].GetString();
            info.m_isVariant = true;
        }

        if (doc.HasMember("GameObject") && doc["GameObject"].IsObject())
        {
            const Value& goNode = doc["GameObject"];
            std::vector<std::string> compNames = { "Transform" };
            if (goNode.HasMember("Components") && goNode["Components"].IsArray())
            {
                for (SizeType i = 0; i < goNode["Components"].Size(); ++i)
                {
                    compNames.push_back(ComponentTypeToString( static_cast<ComponentType>(goNode["Components"][i]["Type"].GetInt())));
                }

            }

            for (size_t i = 0; i < compNames.size(); ++i)
            {
                if (i) {
                    info.m_componentSummary += ", ";
                }
                info.m_componentSummary += compNames[i];
            }
            info.m_childCount = countChildren(countChildren, goNode);
        }
        results.push_back(std::move(info));
    }
    return results;
}

bool PrefabManager::prefabExists(const fs::path& sourcePath)
{
    return fs::exists(sourcePath);
}