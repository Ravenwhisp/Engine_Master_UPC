#pragma once
#include "Module.h"
#include <rapidjson/document.h>
#include "UID.h"
#include "MeshRenderer.h"
#include "SceneSnapshot.h"

class Scene;
class SceneSerializer;

class ModuleScene : public Module
{
private:	
	std::unique_ptr<Scene> m_scene;
	std::unique_ptr<SceneSerializer> m_sceneSerializer;
	std::string m_pendingSceneLoad;

public:
	ModuleScene();
	~ModuleScene();

#pragma region GameLoop
	bool init() override;
	void update() override;
	void render(ID3D12GraphicsCommandList* commandList);
	bool cleanUp() override;
#pragma endregion

#pragma region Persistence
	rapidjson::Value getJSON(rapidjson::Document& domTree);
	void serializeWindowHierarchy(GameObject* gameObject, rapidjson::Value& gameObjectsData, rapidjson::Document& domTree);
	rapidjson::Value getLightingJSON(rapidjson::Document& domTree);
	rapidjson::Value getSkyBoxJSON(rapidjson::Document& domTree);

	bool loadFromJSON(const rapidjson::Value& sceneJson);
	bool loadSceneSkyBox(const rapidjson::Value& sceneJson);
	bool loadSceneLighting(const rapidjson::Value& sceneJson);
	void fixLoadedSceneReferences();
	void resolveDefaultCamera(const rapidjson::Value& sceneJson);

	void saveScene();
	bool loadScene(const std::string& sceneName);
	void requestSceneChange(const std::string& sceneName);
	bool isPendingSceneLoad() const { return !m_pendingSceneLoad.empty(); }
#pragma endregion

	Scene* getScene();
};