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