#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <rapidjson/document.h>

#include "PrefabAsset.h"

class GameObject;
class ModuleScene;

class PrefabManager
{
public:
    struct PrefabInfo
    {
        std::filesystem::path m_sourcePath;
        std::string           m_name;
        std::string           m_componentSummary;
        std::string           m_variantOf;
        UID                   m_uid = 0;   // root GO UID stored in the file
        int                   m_version = 0;
        int                   m_childCount = 0;
        bool                  m_isVariant = false;
    };

    static bool createPrefab(const GameObject* go, const std::filesystem::path& savePath);


    static std::string buildPrefabJSON(const GameObject* go, const std::filesystem::path& savePath);

    static bool applyToPrefab(const GameObject* go, bool respectOverrides = true);
    static bool revertToPrefab(GameObject* go, ModuleScene* scene);

    static bool createVariant(const std::filesystem::path& sourcePath,
        const std::filesystem::path& destinationPath);


    static GameObject* instantiatePrefab(const PrefabAsset& asset, ModuleScene* scene);

    static GameObject* instantiatePrefab(const std::filesystem::path& sourcePath, ModuleScene* scene);

    static void markPropertyOverride(GameObject* go, int componentType, const std::string& propertyName);
    static void clearComponentOverrides(GameObject* go, int componentType);
    static void clearAllOverrides(GameObject* go);
    static void markComponentAdded(GameObject* go, int componentType);
    static void markComponentRemoved(GameObject* go, int componentType);

    static std::string serializeGameObject(const GameObject* go);
    static GameObject* deserializeGameObject(const std::string& data, ModuleScene* scene);

    static bool              isPrefabInstance(const GameObject* go);
    static std::string       getPrefabName(const GameObject* go);
    static UID               getPrefabUID(const GameObject* go);
    static const PrefabData* getInstanceData(const GameObject* go);
    static PrefabData* getInstanceDataMutable(GameObject* go);

    static void linkInstance(GameObject* go, const PrefabData& data);
    static void unlinkInstance(GameObject* go);

    static std::vector<PrefabInfo>            listPrefabsInfo(const std::filesystem::path& searchRoot);
    static std::vector<std::filesystem::path> listPrefabs(const std::filesystem::path& searchRoot);
    static bool                               prefabExists(const std::filesystem::path& sourcePath);

private:
    static bool writePrefabDocument(rapidjson::Document& doc, const std::filesystem::path& path);
    static bool readPrefabDocument(const std::filesystem::path& path, rapidjson::Document& doc);

    static GameObject* deserialiseNode(const rapidjson::Value& node, ModuleScene* scene, GameObject* parent);

    static std::unordered_map<const GameObject*, PrefabData>& registry();
};