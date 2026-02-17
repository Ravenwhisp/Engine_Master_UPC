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
    if (!std::filesystem::exists(SCENE_FOLDER))
    {
        if (std::filesystem::create_directories(SCENE_FOLDER))
        {
            LOG("Created scene folder: %s\n", SCENE_FOLDER.data());
        }
        else
        {
            LOG("Failed to create scene folder: %s\n", SCENE_FOLDER.data());
        }
    }
}


SceneSerializer::~SceneSerializer()
{
}

bool SceneSerializer::SaveScene(std::string sceneName)
{
    if (sceneName.empty())
    {
        LOG("Scene name cannot be empty.\n");
        return false;
	}

    return true;
}


bool SceneSerializer::LoadScene(std::string sceneName) {
    return false;
}