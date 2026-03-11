#pragma once
#include <rapidjson/document.h>

class ModuleScene;

class SceneSerializer
{
public:
    SceneSerializer();
    ~SceneSerializer();

    bool SaveScene(std::string sceneName, rapidjson::Document& domTree);
    bool LoadScene(std::string sceneName);

};
