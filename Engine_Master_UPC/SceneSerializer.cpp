#include "Globals.h"
#include "SceneSerializer.h"

#include <filesystem>
#include <iostream>
#include "simdjson.h"

constexpr std::string_view LOG_TAG = "SceneSerializer";
constexpr std::string_view SCENE_FILE_EXTENSION = ".scene";
constexpr std::string_view SCENE_FOLDER = "Assets/Scenes/";

SceneSerializer::SceneSerializer()
{

}

SceneSerializer::~SceneSerializer()
{
}

bool SceneSerializer::SaveScene(std::string sceneName)
{
    const std::string path = "twitter.json";

    if (!std::filesystem::exists(path))
    {
        LOG("File doesn't exist: %s\n", path.c_str());
        return false;
    }

    simdjson::ondemand::parser parser;

    auto json_result = simdjson::padded_string::load(path);
    if (json_result.error())
    {
        LOG("Error loading JSON\n");
        return false;
    }

    simdjson::padded_string json = std::move(json_result.value());

    auto doc_result = parser.iterate(json);
    if (doc_result.error())
    {
        LOG("Error parsing JSON\n");
        return false;
    }

    simdjson::ondemand::document doc = std::move(doc_result.value());

    auto count_result = doc["search_metadata"]["count"];
    if (count_result.error())
    {
        return false;
    }

    uint64_t count = count_result.get_uint64().value();

    LOG("%llu results.\n", count);

    return true;
}


bool SceneSerializer::LoadScene(std::string sceneName) {
    return false;
}