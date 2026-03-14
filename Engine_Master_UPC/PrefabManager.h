#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rapidjson/document.h>

class GameObject;
class ModuleScene;

struct PrefabOverrideRecord
{
    std::unordered_map<int, std::unordered_set<std::string>> m_modifiedProperties;
    std::vector<int>                                         m_addedComponentTypes;
    std::vector<int>                                         m_removedComponentTypes;

    bool isEmpty() const
    {
        return m_modifiedProperties.empty()
            && m_addedComponentTypes.empty()
            && m_removedComponentTypes.empty();
    }

    void clear()
    {
        m_modifiedProperties.clear();
        m_addedComponentTypes.clear();
        m_removedComponentTypes.clear();
    }
};

struct PrefabInstanceData
{
    std::string          m_prefabName;
    uint32_t             m_prefabUID = 0;
    PrefabOverrideRecord m_overrides;
};

class PrefabManager
{
public:
    struct PrefabInfo
    {
        std::string m_name;
        std::string m_componentSummary;
        std::string m_variantOf;
        uint32_t    m_uid = 0;
        int         m_version = 0;
        int         m_childCount = 0;
        bool        m_isVariant = false;
    };

    static bool         createPrefab(const GameObject* go, const std::string& prefabName);
    static GameObject* instantiatePrefab(const std::string& prefabName, ModuleScene* scene);
    static bool         applyToPrefab(const GameObject* go, bool respectOverrides = true);
    static bool         revertToPrefab(GameObject* go, ModuleScene* scene);
    static bool         createVariant(const std::string& sourcePrefabName, const std::string& destinationPrefabName);

    static void markPropertyOverride(GameObject* go, int componentType, const std::string& propertyName);
    static void clearComponentOverrides(GameObject* go, int componentType);
    static void clearAllOverrides(GameObject* go);

    static void markComponentAdded(GameObject* go, int componentType);
    static void markComponentRemoved(GameObject* go, int componentType);

    static std::string  serializeGameObject(const GameObject* go);
    static GameObject* deserializeGameObject(const std::string& data, ModuleScene* scene);

    static bool                      isPrefabInstance(const GameObject* go);
    static std::string               getPrefabName(const GameObject* go);
    static uint32_t                  getPrefabUID(const GameObject* go);
    static const PrefabInstanceData* getInstanceData(const GameObject* go);
    static PrefabInstanceData* getInstanceDataMutable(GameObject* go);

    static void linkInstance(GameObject* go, const PrefabInstanceData& data);
    static void unlinkInstance(GameObject* go);

    static std::vector<PrefabInfo>   listPrefabsInfo();
    static std::vector<std::string>  listPrefabs();
    static bool                      prefabExists(const std::string& prefabName);

    static uint32_t makePrefabUID(const std::string& name);

private:
    static std::string getPrefabPath(const std::string& name);
    static bool        writePrefabDocument(rapidjson::Document& doc, const std::string& path);
    static bool        readPrefabDocument(const std::string& path, rapidjson::Document& doc);

    struct SerialiseCtx;
    static GameObject* deserialiseNode(const rapidjson::Value& node, ModuleScene* scene, GameObject* parent);

    static std::unordered_map<const GameObject*, PrefabInstanceData>& registry();
};