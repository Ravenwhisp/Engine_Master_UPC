#pragma once
#include "Module.h"
#include "GameObject.h"
#include "Lights.h"
#include "UID.h"

class SceneSerializer;
class Quadtree;

struct SceneDataCB
{
	Vector3 viewPos;
	float pad0 = 0.0f;
};

struct SceneLightingSettings
{
	Vector3 ambientColor;;
	float ambientIntensity;;
};

struct SkyboxSettings
{
	bool enabled = true;
	char path[260] = "Assets/Textures/cubemap2.dds";
};

class SceneModule : public Module
{
private:
	std::string m_name = "SampleScene";

	std::vector<std::unique_ptr<GameObject>> m_allObjects;
	std::vector<GameObject*> m_rootObjects;

	std::unique_ptr<SceneSerializer> m_sceneSerializer;
	std::unique_ptr<Quadtree> m_quadtree;

	SceneLightingSettings		m_lighting;
	SceneDataCB					m_sceneDataCB;
	SkyboxSettings				m_skybox;

public:
	SceneModule();
	~SceneModule();

#pragma region GameLoop
	bool init() override;
	void update() override;
	void preRender() override;
	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix);
	void postRender() override;
	bool cleanUp() override;
#pragma endregion

#pragma region Persistence

	rapidjson::Value getJSON(rapidjson::Document& domTree);
	rapidjson::Value getLightingJSON(rapidjson::Document& domTree);
	rapidjson::Value getSkyboxJSON(rapidjson::Document& domTree);

	bool loadFromJSON(const rapidjson::Value& sceneJson);
	bool loadSceneSkybox(const rapidjson::Value& sceneJson);
	bool loadSceneLighting(const rapidjson::Value& sceneJson);

	void saveScene();
	bool loadScene(const std::string& sceneName);
	void clearScene();
#pragma endregion

	void createGameObject();
	GameObject* createGameObjectWithUID(UID id, UID transformUID);
	void removeGameObject(const UID uuid);
	std::vector<GameObject*> getAllGameObjects();

	void addGameObject(std::unique_ptr<GameObject> gameObject);
	void destroyGameObject(GameObject* gameObject);

	GameObject* findInHierarchy(GameObject* current, UID uuid);
	void destroyHierarchy(GameObject* obj);

	void addToRootList(GameObject* gameObject);
	void removeFromRootList(GameObject* gameObject);
	const std::vector<GameObject*>& getRootObjects() const;

	GameObject* createDirectionalLightOnInit();
	const char* getName() { return (char*)m_name.c_str(); }
	const void setName(const char* newName) { m_name = newName; }

	SceneLightingSettings& GetLightingSettings() { return m_lighting; }
	SceneDataCB& getCBData() { return m_sceneDataCB; }
	SkyboxSettings& getSkyboxSettings() { return m_skybox; }
	const SkyboxSettings& getSkyboxSettings() const { return m_skybox; }

	bool applySkyboxToRenderer();

	Quadtree* getQuadtree() { return m_quadtree.get(); }
	void createQuadtree();
};