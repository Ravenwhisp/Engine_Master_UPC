#include "Globals.h"
#include "PrefabManager.h"

#include "Application.h"
#include "FileSystemModule.h"
#include "GameObject.h"
#include "SceneModule.h"    
#include "Transform.h"     
#include "Component.h"
#include "ComponentType.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"

#include <filesystem>
#include <algorithm>
#include <cassert>
#include <fstream>

using namespace rapidjson;
namespace fs = std::filesystem;

static constexpr int   kPrefabFormatVersion = 2;
static constexpr char  kPrefabDir[] = "Assets/Prefabs/";
static constexpr char  kPrefabExt[] = ".prefab";

struct PrefabManager::SerialiseCtx
{
    Document& doc;
    Document::AllocatorType& a;
    explicit SerialiseCtx(Document& d) : doc(d), a(d.GetAllocator()) {}
};

std::unordered_map<const GameObject*, PrefabInstanceData>& PrefabManager::registry()
{
    static std::unordered_map<const GameObject*, PrefabInstanceData> s_registry;
    return s_registry;
}

uint32_t PrefabManager::makePrefabUID(const std::string& name)
{
    uint32_t hash = 2166136261u;
    for (unsigned char c : name)
    {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash ? hash : 1u;
}

std::string PrefabManager::getPrefabPath(const std::string& name)
{
    return std::string(kPrefabDir) + name + kPrefabExt;
}

bool PrefabManager::writePrefabDocument(Document& doc, const std::string& path)
{
    fs::create_directories(fs::path(path).parent_path());

    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp)
    {
        return false;
    }

    char buf[65536];
    FileWriteStream os(fp, buf, sizeof(buf));
    PrettyWriter<FileWriteStream> writer(os);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);
    fclose(fp);
    return true;
}

bool PrefabManager::readPrefabDocument(const std::string& path, Document& doc)
{
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp)
    {
        return false;
    }

    char buf[65536];
    FileReadStream is(fp, buf, sizeof(buf));
    doc.ParseStream(is);
    fclose(fp);
    return !doc.HasParseError();
}

static void serialiseNodeInto(const GameObject* go, Value& out, Document::AllocatorType& a)
{
    out.SetObject();
    out.AddMember("Name", Value(go->GetName().c_str(), a), a);
    out.AddMember("Active", go->GetActive(), a);

    const PrefabInstanceData* instData = PrefabManager::getInstanceData(go);
    if (instData)
    {
        Value linkNode(kObjectType);
        linkNode.AddMember("PrefabName", Value(instData->prefabName.c_str(), a), a);
        linkNode.AddMember("PrefabUID", instData->prefabUID, a);
        out.AddMember("PrefabLink", linkNode, a);
    }

    {
        const Transform* t = go->GetTransform();
        Value tf(kObjectType);

        const Vector3& pos = t->getPosition();
        const Quaternion& rot = t->getRotation();
        const Vector3& scl = t->getScale();

        Value posArr(kArrayType);
        posArr.PushBack(pos.x, a).PushBack(pos.y, a).PushBack(pos.z, a);

        Value rotArr(kArrayType);
        rotArr.PushBack(rot.x, a).PushBack(rot.y, a).PushBack(rot.z, a).PushBack(rot.w, a);

        Value sclArr(kArrayType);
        sclArr.PushBack(scl.x, a).PushBack(scl.y, a).PushBack(scl.z, a);

        tf.AddMember("position", posArr, a);
        tf.AddMember("rotation", rotArr, a);
        tf.AddMember("scale", sclArr, a);
        out.AddMember("Transform", tf, a);
    }

    {
        Value comps(kArrayType);
        rapidjson::Document helperDoc;
        helperDoc.SetObject();

        for (Component* comp : go->GetAllComponents())
        {
            if (comp->getType() == ComponentType::TRANSFORM)
            {
                continue;
            }

            Value compNode(kObjectType);
            compNode.AddMember("Type", static_cast<int>(comp->getType()), a);

            Value compData = comp->getJSON(helperDoc);
            Value compDataCopy;
            compDataCopy.CopyFrom(compData, a);
            compNode.AddMember("Data", compDataCopy, a);

            comps.PushBack(compNode, a);
        }
        out.AddMember("Components", comps, a);
    }

    {
        Value children(kArrayType);
        for (GameObject* child : go->GetTransform()->getAllChildren())
        {
            Value childNode;
            serialiseNodeInto(child, childNode, a);
            children.PushBack(childNode, a);
        }
        out.AddMember("Children", children, a);
    }
}

GameObject* PrefabManager::deserialiseNode(const Value& node, SceneModule* scene, GameObject* parent)
{
    if (!node.IsObject())
    {
        return nullptr;
    }

    const char* name = node.HasMember("Name") ? node["Name"].GetString() : "Unnamed";

    UID newUID = GenerateUID();
    UID transformUID = GenerateUID();
    GameObject* go = scene->createGameObjectWithUID(newUID, transformUID);

    if (!go)
    {
        DEBUG_ERROR("[PrefabManager] Failed to create GameObject '%s'", name);
        return nullptr;
    }

    go->SetName(name);
    go->SetActive(node.HasMember("Active") ? node["Active"].GetBool() : true);

    if (parent)
    {
        Transform* parentTransform = parent->GetTransform();
        Transform* childTransform = go->GetTransform();
        childTransform->setRoot(parentTransform);
        parentTransform->addChild(go);
        scene->removeFromRootList(go);
    }

    if (node.HasMember("PrefabLink") && node["PrefabLink"].IsObject())
    {
        const Value& lk = node["PrefabLink"];
        PrefabInstanceData linkData;
        linkData.prefabName = lk.HasMember("PrefabName") ? lk["PrefabName"].GetString() : "";
        linkData.prefabUID = lk.HasMember("PrefabUID") ? lk["PrefabUID"].GetUint() : 0;
        if (!linkData.prefabName.empty())
        {
            linkInstance(go, linkData);
        }
    }

    if (node.HasMember("Transform") && node["Transform"].IsObject())
    {
        Transform* t = go->GetTransform();
        const Value& tf = node["Transform"];

        if (tf.HasMember("position") && tf["position"].IsArray())
        {
            const auto& p = tf["position"];
            t->setPosition(Vector3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat()));
        }
        if (tf.HasMember("rotation") && tf["rotation"].IsArray())
        {
            const auto& r = tf["rotation"];
            t->setRotation(Quaternion(r[0].GetFloat(), r[1].GetFloat(), r[2].GetFloat(), r[3].GetFloat()));
        }
        if (tf.HasMember("scale") && tf["scale"].IsArray())
        {
            const auto& s = tf["scale"];
            t->setScale(Vector3(s[0].GetFloat(), s[1].GetFloat(), s[2].GetFloat()));
        }
    }

    if (node.HasMember("Components") && node["Components"].IsArray())
    {
        for (SizeType i = 0; i < node["Components"].Size(); ++i)
        {
            const Value& cn = node["Components"][i];
            auto         type = static_cast<ComponentType>(cn["Type"].GetInt());
            Component* comp = go->AddComponentWithUID(type, GenerateUID());

            if (comp)
            {
                if (cn.HasMember("Data") && cn["Data"].IsObject())
                {
                    comp->deserializeJSON(cn["Data"]);
                }
            }
            else
            {
                DEBUG_WARN("[PrefabManager] Could not create component type %d for '%s'",
                    cn["Type"].GetInt(), name);
            }
        }
    }

    if (node.HasMember("Children") && node["Children"].IsArray())
    {
        for (SizeType i = 0; i < node["Children"].Size(); ++i)
        {
            deserialiseNode(node["Children"][i], scene, go);
        }
    }

    return go;
}

void PrefabManager::serialiseNode(const GameObject* go, SerialiseCtx& ctx)
{
    (void)go;
    (void)ctx;
    assert(false && "Use serialiseNodeInto() instead");
}

std::string PrefabManager::serializeGameObject(const GameObject* go)
{
    if (!go)
    {
        return {};
    }

    Document doc;
    doc.SetObject();
    auto& a = doc.GetAllocator();

    Value goNode;
    serialiseNodeInto(go, goNode, a);
    doc.AddMember("GameObject", goNode, a);

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}

GameObject* PrefabManager::deserializeGameObject(const std::string& data, SceneModule* scene)
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
    const PrefabInstanceData* d = getInstanceData(go);
    return d ? d->prefabName : "";
}

uint32_t PrefabManager::getPrefabUID(const GameObject* go)
{
    const PrefabInstanceData* d = getInstanceData(go);
    return d ? d->prefabUID : 0;
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
    PrefabInstanceData* inst = getInstanceDataMutable(go);
    if (inst)
    {
        inst->overrides.modifiedProperties[componentType].insert(propertyName);
    }
}

void PrefabManager::clearComponentOverrides(GameObject* go, int componentType)
{
    PrefabInstanceData* inst = getInstanceDataMutable(go);
    if (!inst)
    {
        return;
    }

    inst->overrides.modifiedProperties.erase(componentType);

    auto& av = inst->overrides.addedComponentTypes;
    av.erase(std::remove(av.begin(), av.end(), componentType), av.end());

    auto& rv = inst->overrides.removedComponentTypes;
    rv.erase(std::remove(rv.begin(), rv.end(), componentType), rv.end());
}

void PrefabManager::clearAllOverrides(GameObject* go)
{
    PrefabInstanceData* inst = getInstanceDataMutable(go);
    if (inst)
    {
        inst->overrides.clear();
    }
}

bool PrefabManager::createPrefab(const GameObject* go, const std::string& prefabName)
{
    if (!go || prefabName.empty())
    {
        DEBUG_ERROR("[PrefabManager] createPrefab: null go or empty name");
        return false;
    }

    std::error_code ec;
    fs::create_directories(kPrefabDir, ec);

    Document doc;
    doc.SetObject();
    auto& a = doc.GetAllocator();

    doc.AddMember("PrefabName", Value(prefabName.c_str(), a), a);
    doc.AddMember("Version", kPrefabFormatVersion, a);
    doc.AddMember("PrefabUID", makePrefabUID(prefabName), a);

    const PrefabInstanceData* instData = getInstanceData(go);
    if (instData && !instData->prefabName.empty() && instData->prefabName != prefabName)
    {
        doc.AddMember("VariantOf", Value(instData->prefabName.c_str(), a), a);
    }

    Value goNode;
    serialiseNodeInto(go, goNode, a);
    doc.AddMember("GameObject", goNode, a);

    std::string path = getPrefabPath(prefabName);
    if (!writePrefabDocument(doc, path))
    {
        DEBUG_ERROR("[PrefabManager] createPrefab: Cannot write to '%s'", path.c_str());
        return false;
    }

    app->getFileSystemModule()->rebuild();

    DEBUG_LOG("[PrefabManager] Saved prefab '%s' -> %s", prefabName.c_str(), path.c_str());
    return true;
}

GameObject* PrefabManager::instantiatePrefab(const std::string& prefabName, SceneModule* scene)
{
    if (!scene || prefabName.empty())
    {
        return nullptr;
    }

    std::string path = getPrefabPath(prefabName);
    if (!fs::exists(path))
    {
        DEBUG_ERROR("[PrefabManager] instantiatePrefab: Prefab not found: %s", path.c_str());
        return nullptr;
    }

    Document doc;
    if (!readPrefabDocument(path, doc))
    {
        DEBUG_ERROR("[PrefabManager] instantiatePrefab: JSON parse error in '%s'", path.c_str());
        return nullptr;
    }

    if (!doc.HasMember("GameObject") || !doc["GameObject"].IsObject())
    {
        DEBUG_ERROR("[PrefabManager] instantiatePrefab: Missing 'GameObject' in '%s'", path.c_str());
        return nullptr;
    }

    GameObject* go = deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go)
    {
        return nullptr;
    }

    PrefabInstanceData instData;
    instData.prefabName = prefabName;
    instData.prefabUID = doc.HasMember("PrefabUID") ? doc["PrefabUID"].GetUint() : makePrefabUID(prefabName);
    linkInstance(go, instData);

    DEBUG_LOG("[PrefabManager] Instantiated prefab '%s' -> GO '%s'", prefabName.c_str(), go->GetName().c_str());
    return go;
}

bool PrefabManager::applyToPrefab(const GameObject* go, bool respectOverrides)
{
    const PrefabInstanceData* inst = getInstanceData(go);
    if (!inst || inst->prefabName.empty())
    {
        DEBUG_ERROR("[PrefabManager] applyToPrefab: '%s' is not a prefab instance",
            go ? go->GetName().c_str() : "null");
        return false;
    }

    if (!respectOverrides)
    {
        return createPrefab(go, inst->prefabName);
    }

    fs::create_directories(kPrefabDir);

    Document doc;
    doc.SetObject();
    auto& a = doc.GetAllocator();

    doc.AddMember("PrefabName", Value(inst->prefabName.c_str(), a), a);
    doc.AddMember("Version", kPrefabFormatVersion, a);
    doc.AddMember("PrefabUID", inst->prefabUID, a);

    if (!inst->overrides.isEmpty())
    {
        Value overrideMap(kObjectType);

        Value modProps(kObjectType);
        for (const auto& [compType, props] : inst->overrides.modifiedProperties)
        {
            Value propArr(kArrayType);
            for (const auto& p : props)
            {
                propArr.PushBack(Value(p.c_str(), a), a);
            }
            std::string key = std::to_string(compType);
            modProps.AddMember(Value(key.c_str(), a), propArr, a);
        }
        overrideMap.AddMember("ModifiedProperties", modProps, a);

        Value added(kArrayType);
        for (int t : inst->overrides.addedComponentTypes)
        {
            added.PushBack(t, a);
        }
        overrideMap.AddMember("AddedComponents", added, a);

        Value removed(kArrayType);
        for (int t : inst->overrides.removedComponentTypes)
        {
            removed.PushBack(t, a);
        }
        overrideMap.AddMember("RemovedComponents", removed, a);

        doc.AddMember("OverrideMap", overrideMap, a);
    }

    Value goNode;
    serialiseNodeInto(go, goNode, a);
    doc.AddMember("GameObject", goNode, a);

    std::string path = getPrefabPath(inst->prefabName);
    if (!writePrefabDocument(doc, path))
    {
        DEBUG_ERROR("[PrefabManager] applyToPrefab: Cannot write to '%s'", path.c_str());
        return false;
    }

    DEBUG_LOG("[PrefabManager] Applied instance '%s' -> prefab '%s'",
        go->GetName().c_str(), inst->prefabName.c_str());
    return true;
}

bool PrefabManager::revertToPrefab(GameObject* go, SceneModule* scene)
{

    PrefabInstanceData* inst = getInstanceDataMutable(go);
    if (!inst || inst->prefabName.empty())
    {
        DEBUG_ERROR("[PrefabManager] revertToPrefab: '%s' is not a prefab instance",
            go ? go->GetName().c_str() : "null");
        return false;
    }

    std::string path = getPrefabPath(inst->prefabName);
    if (!fs::exists(path))
    {
        DEBUG_ERROR("[PrefabManager] revertToPrefab: Prefab file missing: %s", path.c_str());
        return false;
    }

    Document doc;
    if (!readPrefabDocument(path, doc) || !doc.HasMember("GameObject"))
    {
        return false;
    }

    const Value& node = doc["GameObject"];
    PrefabOverrideRecord savedOverrides = inst->overrides;
    const int kTransformType = static_cast<int>(ComponentType::TRANSFORM);

    if (node.HasMember("Transform") && node["Transform"].IsObject())
    {
        Transform* t = go->GetTransform();
        const Value& tf = node["Transform"];

        auto ovIt = savedOverrides.modifiedProperties.find(kTransformType);
        const std::unordered_set<std::string>* ovSet =
            (ovIt != savedOverrides.modifiedProperties.end()) ? &ovIt->second : nullptr;
        auto overridden = [&](const char* prop) { return ovSet && ovSet->count(prop) > 0; };

        if (!overridden("position") && tf.HasMember("position") && tf["position"].IsArray())
        {
            const auto& p = tf["position"];
            t->setPosition(Vector3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat()));
        }
        if (!overridden("rotation") && tf.HasMember("rotation") && tf["rotation"].IsArray())
        {
            const auto& r = tf["rotation"];
            t->setRotation(Quaternion(r[0].GetFloat(), r[1].GetFloat(), r[2].GetFloat(), r[3].GetFloat()));
        }
        if (!overridden("scale") && tf.HasMember("scale") && tf["scale"].IsArray())
        {
            const auto& s = tf["scale"];
            t->setScale(Vector3(s[0].GetFloat(), s[1].GetFloat(), s[2].GetFloat()));
        }
        t->markDirty();
    }

    DEBUG_WARN("[PrefabManager] revertToPrefab: component revert not yet implemented "
        "(transform reverted successfully)");

    inst->overrides = savedOverrides;
    DEBUG_LOG("[PrefabManager] Reverted transform of '%s' from prefab '%s'",
        go->GetName().c_str(), inst->prefabName.c_str());
    return true;
}

bool PrefabManager::createVariant(const std::string& srcPrefabName, const std::string& dstPrefabName)
{
    if (srcPrefabName.empty() || dstPrefabName.empty())
    {
        return false;
    }

    std::string srcPath = getPrefabPath(srcPrefabName);
    if (!fs::exists(srcPath))
    {
        DEBUG_ERROR("[PrefabManager] createVariant: Source not found: %s", srcPath.c_str());
        return false;
    }

    fs::create_directories(kPrefabDir);

    Document doc;
    if (!readPrefabDocument(srcPath, doc))
    {
        return false;
    }

    auto& a = doc.GetAllocator();
    auto setOrAdd = [&](const char* key, const std::string& val)
        {
            if (doc.HasMember(key))
            {
                doc[key].SetString(val.c_str(), a);
            }
            else
            {
                doc.AddMember(Value(key, a), Value(val.c_str(), a), a);
            }
        };

    setOrAdd("PrefabName", dstPrefabName);
    setOrAdd("VariantOf", srcPrefabName);

    if (doc.HasMember("PrefabUID"))
    {
        doc["PrefabUID"].SetUint(makePrefabUID(dstPrefabName));
    }
    else
    {
        doc.AddMember("PrefabUID", makePrefabUID(dstPrefabName), a);
    }

    if (!writePrefabDocument(doc, getPrefabPath(dstPrefabName)))
    {
        return false;
    }

    DEBUG_LOG("[PrefabManager] Created variant '%s' from '%s'",
        dstPrefabName.c_str(), srcPrefabName.c_str());
    return true;
}

std::vector<std::string> PrefabManager::listPrefabs()
{
    std::vector<std::string> names;
    if (!fs::exists(kPrefabDir))
    {
        return names;
    }

    try
    {
        for (const auto& e : fs::directory_iterator(kPrefabDir))
        {
            if (e.is_regular_file() && e.path().extension() == kPrefabExt)
            {
                names.push_back(e.path().stem().string());
            }
        }
    }
    catch (...) {}

    return names;
}

std::vector<PrefabManager::PrefabInfo> PrefabManager::listPrefabsInfo()
{
    std::vector<PrefabInfo> results;
    if (!fs::exists(kPrefabDir))
    {
        return results;
    }

    try
    {
        for (const auto& entry : fs::directory_iterator(kPrefabDir))
        {
            if (!entry.is_regular_file() || entry.path().extension() != kPrefabExt)
            {
                continue;
            }

            PrefabInfo info;
            info.name = entry.path().stem().string();

            Document doc;
            if (!readPrefabDocument(entry.path().string(), doc))
            {
                continue;
            }

            info.uid = doc.HasMember("PrefabUID") ? doc["PrefabUID"].GetUint() : 0;
            info.version = doc.HasMember("Version") ? doc["Version"].GetInt() : 0;

            if (doc.HasMember("VariantOf") && doc["VariantOf"].IsString())
            {
                info.variantOf = doc["VariantOf"].GetString();
                info.isVariant = true;
            }

            if (doc.HasMember("GameObject") && doc["GameObject"].IsObject())
            {
                const Value& gn = doc["GameObject"];

                std::vector<std::string> compNames = { "Transform" };
                if (gn.HasMember("Components") && gn["Components"].IsArray())
                {
                    for (SizeType i = 0; i < gn["Components"].Size(); ++i)
                    {
                        auto type = static_cast<ComponentType>(gn["Components"][i]["Type"].GetInt());
                        compNames.push_back(ComponentTypeToString(type));
                    }
                }

                for (size_t i = 0; i < compNames.size(); ++i)
                {
                    if (i) info.componentSummary += ", ";
                    info.componentSummary += compNames[i];
                }

                std::function<int(const Value&)> countChildren = [&](const Value& n) -> int
                    {
                        int c = 0;
                        if (n.HasMember("Children") && n["Children"].IsArray())
                        {
                            c += static_cast<int>(n["Children"].Size());
                            for (SizeType i = 0; i < n["Children"].Size(); ++i)
                            {
                                c += countChildren(n["Children"][i]);
                            }
                        }
                        return c;
                    };

                info.childCount = countChildren(gn);
            }

            results.push_back(std::move(info));
        }
    }
    catch (...) {}

    return results;
}

bool PrefabManager::prefabExists(const std::string& prefabName)
{
    return fs::exists(getPrefabPath(prefabName));
}