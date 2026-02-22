#pragma once
#include <rapidjson/document.h>

class SceneModule;

class SceneSerializer
{
public:
    SceneSerializer();
    ~SceneSerializer();

    bool SaveScene(std::string sceneName);
    bool LoadScene(std::string sceneName);

    bool loadSceneSkybox(SceneModule* sceneModule, const rapidjson::Value& sceneJson);
    bool loadSceneLighting(SceneModule* sceneModule, const rapidjson::Value& sceneJson);

};
