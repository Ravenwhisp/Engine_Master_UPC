#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "UID.h"
#include "PrefabAsset.h" 

#include <rapidjson/document.h>

class GameObject;
class Scene;

// PrefabInfo / PrefabOverrideRecord / PrefabData are now owned by GameObject
class PrefabManager
{
public:
    struct PrefabFileInfo
    {
        std::filesystem::path m_sourcePath;
        std::string m_name;
        std::string m_componentSummary;
        std::string m_variantOf;
        UID m_uid = 0;
        int m_version = 0;
        int m_childCount = 0;
        bool m_isVariant = false;
    };

    static std::vector<PrefabFileInfo> listPrefabsInfo(const std::filesystem::path& searchRoot);
    static std::vector<std::filesystem::path> listPrefabs(const std::filesystem::path& searchRoot);

    static bool createPrefab(GameObject* go, const std::filesystem::path& savePath);
    static bool applyToPrefab(const GameObject* go, bool respectOverrides = true);
    static bool revertToPrefab(GameObject* go, Scene* scene);
    static bool createVariant(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath);
    static void refreshInstances(const std::filesystem::path& prefabPath);

    static std::string buildPrefabJSON(const GameObject* go, const std::filesystem::path& savePath);

    static GameObject* instantiatePrefab(const PrefabAsset& asset, Scene* scene);
    static GameObject* instantiatePrefab(const std::filesystem::path& sourcePath, Scene* scene);

private:
    static bool writePrefabDocument(rapidjson::Document& doc, const std::filesystem::path& path);
    static bool readPrefabDocument(const std::filesystem::path& path, rapidjson::Document& doc);
    static bool loadDocument(const std::filesystem::path& path, rapidjson::Document& doc);
    static GameObject* deserialiseNode(const rapidjson::Value& node, Scene* scene, GameObject* parent);
};