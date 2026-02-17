#pragma once

class SceneSerializer
{
public:
    SceneSerializer();
    ~SceneSerializer();

    bool SaveScene(std::string sceneName);
    bool LoadScene(std::string sceneName);
};

