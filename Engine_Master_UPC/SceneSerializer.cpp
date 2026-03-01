#include "Globals.h"
#include "SceneSerializer.h"

#include <filesystem>
#include <iostream>

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "Application.h"
#include "SceneModule.h"
#include "GameObject.h"
#include "Transform.h"
#include "Component.h"
#include "ComponentType.h"

#include <rapidjson/document.h>
#include "rapidjson/filewritestream.h"
#include <rapidjson/writer.h>


constexpr std::string_view LOG_TAG = "SceneSerializer";
constexpr std::string_view SCENE_FILE_EXTENSION = ".scene";
constexpr std::string_view SCENE_FOLDER = "Assets/Scenes/";

SceneSerializer::SceneSerializer()
{
    if (!std::filesystem::exists(SCENE_FOLDER))
    {
        if (std::filesystem::create_directories(SCENE_FOLDER))
        {
            DEBUG_LOG("Created scene folder: %s\n", SCENE_FOLDER.data());
        }
        else
        {
            DEBUG_ERROR("Failed to create scene folder: %s\n", SCENE_FOLDER.data());
        }
    }
}


SceneSerializer::~SceneSerializer()
{
}

bool SceneSerializer::SaveScene(std::string sceneName, rapidjson::Document& domTree)
{
    if (sceneName.empty())
    {
        DEBUG_LOG("Scene name cannot be empty.\n");
        return false;
	}

    const std::string path = std::string(SCENE_FOLDER) + sceneName + std::string(SCENE_FILE_EXTENSION);

    // Save file //

    FILE* fileOpened = std::fopen(path.c_str(), "wb"); // w for writing, b disables special handling of '\n' and '\x1A'
    if (!fileOpened) 
    {
        DEBUG_ERROR("Error opening file");
        return false;
    }

    // Create a FileWriteStream
    char writeBuffer[65536];
    rapidjson::FileWriteStream stream(fileOpened, writeBuffer, sizeof(writeBuffer));

    // Write JSON to file
    rapidjson::Writer<rapidjson::FileWriteStream> writer(stream);
    domTree.Accept(writer);

    // Close file
    std::fclose(fileOpened);

    return true;
}


bool SceneSerializer::LoadScene(std::string sceneName)
{
    if (sceneName.empty()) 
    {
        return false;
    }

    const std::string path = std::string(SCENE_FOLDER) + sceneName + std::string(SCENE_FILE_EXTENSION);

    // Read file
    std::ifstream file(path, std::ios::binary);
    if (!file) 
    {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string json = ss.str();

    // Parse JSON
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError()) 
    {
        return false;
    }

    const rapidjson::Value& sceneJson = doc[sceneName.c_str()];
    SceneModule* sceneModule = app->getSceneModule();

    return sceneModule->loadFromJSON(sceneJson);

}
