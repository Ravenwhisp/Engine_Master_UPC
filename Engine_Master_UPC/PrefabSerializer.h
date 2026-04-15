// PrefabSerializer.h
#pragma once
#include <filesystem>
#include <string>
#include <rapidjson/document.h>

class GameObject;
class Scene;

class PrefabSerializer
{
public:
    // JSON ? in-memory
    static GameObject* deserialiseNode(const rapidjson::Value& node, Scene* scene, GameObject* parent);

    // in-memory ? JSON string (used by ModuleAssets::savePrefab)
    static std::string buildPrefabJSON(const GameObject* go, const std::filesystem::path& savePath);

    // Writes a fully-formed Document to disk
    static bool writeDocument(rapidjson::Document& doc, const std::filesystem::path& path);

    // Reads a Document from disk; returns false on parse error
    static bool readDocument(const std::filesystem::path& path, rapidjson::Document& doc);

    // readDocument + validates "GameObject" member exists
    static bool loadDocument(const std::filesystem::path& path, rapidjson::Document& doc);

    // Fills a Document with the standard prefab header fields
    static void buildDocumentHeader(rapidjson::Document& doc,
        const GameObject* go,
        const std::filesystem::path& savePath);

private:
    static void serialiseNodeInto(const GameObject* go,
        rapidjson::Value& out,
        rapidjson::Document::AllocatorType& alloc);
    static void serialiseTransform(const GameObject* go,
        rapidjson::Value& out,
        rapidjson::Document::AllocatorType& alloc);
    static void serialiseComponents(const GameObject* go,
        rapidjson::Value& out,
        rapidjson::Document::AllocatorType& alloc);
    static void deserialiseTransform(const rapidjson::Value& node, GameObject* go);
    static void deserialiseComponents(const rapidjson::Value& node, GameObject* go);
};