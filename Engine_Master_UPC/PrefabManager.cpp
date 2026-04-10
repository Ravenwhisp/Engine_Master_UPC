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
#include "PrefabAsset.h"

using namespace rapidjson;
namespace fs = std::filesystem;

static constexpr int         PREFAB_FORMAT_VERSION = 2;
static constexpr const char* PREFAB_EXT = ".prefab";

bool PrefabManager::writePrefabDocument(Document& doc, const fs::path& path)
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

bool PrefabManager::readPrefabDocument(const fs::path& path, Document& doc)
{
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file) return false;

    char buf[65536];
    FileReadStream is(file, buf, sizeof(buf));
    doc.ParseStream(is);
    fclose(file);
    return !doc.HasParseError();
}

bool PrefabManager::loadDocument(const fs::path& path, Document& doc)
{
    return readPrefabDocument(path, doc)
        && doc.HasMember("GameObject")
        && doc["GameObject"].IsObject();
}

static void serialiseTransform(const GameObject* go, Value& out, Document::AllocatorType& alloc)
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

static void serialiseComponents(const GameObject* go, Value& out, Document::AllocatorType& alloc)
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

static void serialiseNodeInto(const GameObject* go, Value& out, Document::AllocatorType& alloc)
{
    out.SetObject();
    out.AddMember("Name", Value(go->GetName().c_str(), alloc), alloc);
    out.AddMember("Active", go->GetActive(), alloc);

    const PrefabInfo& info = go->GetPrefabInfo();
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
        tf->setRotation(Quaternion(r[0].GetFloat(), r[1].GetFloat(), r[2].GetFloat(), r[3].GetFloat()));
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
            comp->deserializeJSON(cn["Data"]);
    }
}

GameObject* PrefabManager::deserialiseNode(const Value& node, Scene* scene, GameObject* parent)
{
    if (!node.IsObject()) return nullptr;

    GameObject* go = scene->createGameObjectWithUID(GenerateUID(), GenerateUID());
    if (!go) return nullptr;

    go->SetName(node.HasMember("Name") ? node["Name"].GetString() : "Unnamed");
    go->SetActive(node.HasMember("Active") ? node["Active"].GetBool() : true);

    if (parent)
    {
        go->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(go);
        scene->removeFromRootList(go);
    }

    if (node.HasMember("PrefabLink") && node["PrefabLink"].IsObject())
    {
        const Value& pl = node["PrefabLink"];
        PrefabInfo& info = go->GetPrefabInfo();
        // Old files may contain PrefabUID/PrefabName keys — ignore them.
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

    return go;
}

// Builds a prefab Document header (SourcePath, Name, Version, optional VariantOf).
// The caller adds the "GameObject" member and any other fields on top.
static void buildDocumentHeader(Document& doc, const GameObject* go, const fs::path& savePath)
{
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    const std::string pathStr = savePath.string();
    const std::string name = savePath.stem().string();

    doc.AddMember("SourcePath", Value(pathStr.c_str(), alloc), alloc);
    doc.AddMember("Name", Value(name.c_str(), alloc), alloc);
    doc.AddMember("Version", PREFAB_FORMAT_VERSION, alloc);

    const PrefabInfo& info = go->GetPrefabInfo();
    if (info.isInstance() && info.m_sourcePath != savePath)
        doc.AddMember("VariantOf", Value(info.m_sourcePath.string().c_str(), alloc), alloc);
}

// Pushes the in-memory JSON string into the asset cache entry for savePath,
// so that future instantiates use the new data without a disk read.
static void updateAssetCache(const fs::path& savePath, const Document& doc)
{
    auto asset = app->getModuleAssets()->loadAtPath<PrefabAsset>(savePath);
    if (!asset) return;

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    doc.Accept(writer);
    asset->getData().m_json = sb.GetString();
}

std::string PrefabManager::buildPrefabJSON(const GameObject* go, const fs::path& savePath)
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

bool PrefabManager::createPrefab(GameObject* go, const fs::path& savePath)
{
    if (!go || savePath.empty()) return false;

    Document doc;
    buildDocumentHeader(doc, go, savePath);

    Value goNode;
    serialiseNodeInto(go, goNode, doc.GetAllocator());
    doc.AddMember("GameObject", goNode, doc.GetAllocator());

    if (!writePrefabDocument(doc, savePath)) return false;

    go->GetPrefabInfo().m_sourcePath = savePath;

    updateAssetCache(savePath, doc);
    refreshInstances(savePath);
    return true;
}

bool PrefabManager::applyToPrefab(const GameObject* go, bool respectOverrides)
{
    const PrefabInfo& info = go->GetPrefabInfo();
    if (!info.isInstance()) return false;

    // Without override tracking just re-serialise the whole GO.
    if (!respectOverrides)
        return createPrefab(const_cast<GameObject*>(go), info.m_sourcePath);

    Document doc;
    buildDocumentHeader(doc, go, info.m_sourcePath);
    auto& alloc = doc.GetAllocator();

    if (!info.m_overrides.isEmpty())
    {
        Value overrideMap(kObjectType);

        Value modProps(kObjectType);
        for (const auto& [ct, props] : info.m_overrides.m_modifiedProperties)
        {
            Value arr(kArrayType);
            for (const auto& p : props)
                arr.PushBack(Value(p.c_str(), alloc), alloc);
            const std::string key = std::to_string(ct);
            modProps.AddMember(Value(key.c_str(), alloc), arr, alloc);
        }
        overrideMap.AddMember("ModifiedProperties", modProps, alloc);

        Value addedArr(kArrayType);
        for (int t : info.m_overrides.m_addedComponentTypes)
            addedArr.PushBack(t, alloc);
        overrideMap.AddMember("AddedComponents", addedArr, alloc);

        Value removedArr(kArrayType);
        for (int t : info.m_overrides.m_removedComponentTypes)
            removedArr.PushBack(t, alloc);
        overrideMap.AddMember("RemovedComponents", removedArr, alloc);

        doc.AddMember("OverrideMap", overrideMap, alloc);
    }

    Value goNode;
    serialiseNodeInto(go, goNode, alloc);
    doc.AddMember("GameObject", goNode, alloc);

    if (!writePrefabDocument(doc, info.m_sourcePath)) return false;

    updateAssetCache(info.m_sourcePath, doc);
    refreshInstances(info.m_sourcePath);
    return true;
}

bool PrefabManager::revertToPrefab(GameObject* go, Scene* scene)
{
    PrefabInfo& info = go->GetPrefabInfo();
    if (!info.isInstance()) return false;

    // Load the prefab document — prefer the asset cache to avoid a disk read.
    Document doc;
    bool loaded = false;
    if (isValidAsset(info.m_assetUID))
    {
        auto asset = app->getModuleAssets()->load<PrefabAsset>(info.m_assetUID);
        if (asset && !asset->getJSON().empty())
        {
            doc.Parse(asset->getJSON().c_str());
            loaded = !doc.HasParseError() && doc.HasMember("GameObject");
        }
    }
    if (!loaded)
        loaded = loadDocument(info.m_sourcePath, doc);
    if (!loaded) return false;

    const Value& goNode = doc["GameObject"];
    const PrefabOverrideRecord savedOverrides = info.m_overrides;
    const int             transformType = static_cast<int>(ComponentType::TRANSFORM);

    // Restore transform, skipping any properties that were intentionally overridden.
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

    // Restore components, skipping any that were intentionally overridden.
    if (goNode.HasMember("Components") && goNode["Components"].IsArray())
    {
        for (SizeType i = 0; i < goNode["Components"].Size(); ++i)
        {
            const Value& cn = goNode["Components"][i];
            if (!cn.HasMember("Type") || !cn.HasMember("Data")) continue;

            const int ct = cn["Type"].GetInt();
            auto      oit = savedOverrides.m_modifiedProperties.find(ct);
            if (oit != savedOverrides.m_modifiedProperties.end() && oit->second.count("properties") > 0)
                continue;

            Component* comp = go->GetComponent(static_cast<ComponentType>(ct));
            if (comp) comp->deserializeJSON(cn["Data"]);
        }
    }

    info.m_overrides = savedOverrides;
    return true;
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

    const auto& assetData = asset.getData();
    PrefabInfo& info = go->GetPrefabInfo();
    info.m_sourcePath = assetData.m_sourcePath;
    info.m_assetUID = assetData.m_assetUID;

    return go;
}

GameObject* PrefabManager::instantiatePrefab(const fs::path& sourcePath, Scene* scene)
{
    if (!scene || sourcePath.empty()) return nullptr;

    // Prefer the asset cache so we don't hit disk on every instantiate.
    auto asset = app->getModuleAssets()->loadAtPath<PrefabAsset>(sourcePath);
    if (asset)
        return instantiatePrefab(*asset, scene);

    Document doc;
    if (!loadDocument(sourcePath, doc)) return nullptr;

    GameObject* go = deserialiseNode(doc["GameObject"], scene, nullptr);
    if (!go) return nullptr;

    go->GetPrefabInfo().m_sourcePath = sourcePath;
    return go;
}

void PrefabManager::refreshInstances(const fs::path& prefabPath)
{
    if (prefabPath.empty()) return;

    Scene* scene = app->getModuleScene()->getScene();
    if (!scene) return;

    for (GameObject* go : scene->getAllGameObjects())
    {
        if (go && go->GetPrefabInfo().isInstance() && go->GetPrefabInfo().m_sourcePath == prefabPath)
            revertToPrefab(go, scene);
    }
}

bool PrefabManager::createVariant(const fs::path& sourcePath, const fs::path& destinationPath)
{
    if (sourcePath.empty() || destinationPath.empty()) return false;

    Document doc;
    if (!readPrefabDocument(sourcePath, doc)) return false;
    auto& alloc = doc.GetAllocator();

    auto setOrAdd = [&](const char* key, const std::string& value)
        {
            if (doc.HasMember(key))
                doc[key].SetString(value.c_str(), alloc);
            else
                doc.AddMember(Value(key, alloc), Value(value.c_str(), alloc), alloc);
        };

    setOrAdd("SourcePath", destinationPath.string());
    setOrAdd("Name", destinationPath.stem().string());
    setOrAdd("VariantOf", sourcePath.string());

    return writePrefabDocument(doc, destinationPath);
}

std::vector<fs::path> PrefabManager::listPrefabs(const fs::path& searchRoot)
{
    std::vector<fs::path> paths;
    if (!fs::exists(searchRoot)) return paths;

    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(searchRoot, ec))
    {
        if (entry.is_regular_file() && entry.path().extension() == PREFAB_EXT)
            paths.push_back(entry.path());
    }
    return paths;
}

std::vector<PrefabManager::PrefabFileInfo> PrefabManager::listPrefabsInfo(const fs::path& searchRoot)
{
    std::vector<PrefabFileInfo> results;
    if (!fs::exists(searchRoot)) return results;

    // Recursively counts all descendant GameObjects under a node.
    auto countChildren = [](auto& self, const Value& node) -> int
        {
            int count = 0;
            if (node.HasMember("Children") && node["Children"].IsArray())
            {
                count += static_cast<int>(node["Children"].Size());
                for (SizeType i = 0; i < node["Children"].Size(); ++i)
                    count += self(self, node["Children"][i]);
            }
            return count;
        };

    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(searchRoot, ec))
    {
        if (!entry.is_regular_file() || entry.path().extension() != PREFAB_EXT) continue;

        Document doc;
        if (!readPrefabDocument(entry.path(), doc)) continue;

        PrefabFileInfo info;
        info.m_sourcePath = entry.path();
        info.m_name = entry.path().stem().string();
        info.m_version = doc.HasMember("Version") ? doc["Version"].GetInt() : 0;

        if (doc.HasMember("PrefabUID") && doc["PrefabUID"].IsUint64())
            info.m_uid = static_cast<UID>(doc["PrefabUID"].GetUint64());

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
                    compNames.push_back(ComponentTypeToString(
                        static_cast<ComponentType>(goNode["Components"][i]["Type"].GetInt())));
            }

            for (size_t i = 0; i < compNames.size(); ++i)
            {
                if (i) info.m_componentSummary += ", ";
                info.m_componentSummary += compNames[i];
            }
            info.m_childCount = countChildren(countChildren, goNode);
        }

        results.push_back(std::move(info));
    }
    return results;
}