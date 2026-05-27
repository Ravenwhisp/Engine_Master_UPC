#pragma once
#include <filesystem>
#include <string>
#include <rapidjson/document.h>

class GameObject;
class Scene;

class PrefabSerializer
{
public:
    static GameObject* deserialiseNode(const rapidjson::Value& node, Scene* scene, GameObject* parent);
    static GameObject* deserialiseNode(const rapidjson::Value& node, GameObject* parent);

    static void deserialiseTransform(const rapidjson::Value& node, GameObject* go);
    static void deserialiseComponents(const rapidjson::Value& node, GameObject* go);

    static std::string buildPrefabJSON(const GameObject* go, const std::filesystem::path& savePath);

    static void buildDocumentHeader(rapidjson::Document& doc, const GameObject* go, const std::filesystem::path& savePath);
};
