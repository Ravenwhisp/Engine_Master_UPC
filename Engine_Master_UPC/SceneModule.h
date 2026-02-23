#pragma once
#include "Module.h"
#include "GameObject.h"
#include "Quadtree.h"
#include "Lights.h"
#include "UID.h"

class SceneSerializer;

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
	SceneSerializer* m_sceneSerializer;

public:
#pragma region GameLoop
	bool init() override;
	void update() override;
	void updateHierarchy(GameObject* obj);
	void preRender() override;
	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix);
	void postRender() override;
	bool cleanUp() override;
#pragma endregion

#pragma region Persistence

	rapidjson::Value getJSON(rapidjson::Document& domTree);
	rapidjson::Value getLightingJSON(rapidjson::Document& domTree);
	rapidjson::Value getSkyboxJSON(rapidjson::Document& domTree);

	void saveScene();
	void loadScene();
	void clearScene();
#pragma endregion

	void createGameObject();
	GameObject* createGameObjectWithUID(UID id);
	void removeGameObject(const UID uuid);

	void addGameObject(GameObject* gameObject);
	void detachGameObject(GameObject* gameObject);
	void destroyGameObject(GameObject* gameObject);

	GameObject* findInHierarchy(GameObject* current, UID uuid);
	void destroyHierarchy(GameObject* obj);

	GameObject* createDirectionalLightOnInit();

	const std::vector<GameObject*>& getAllGameObjects() { return m_gameObjects; }

	const char* getName() { return (char*)m_name.c_str(); }
	const void setName(const char* newName) { m_name = newName; }

	SceneLightingSettings& GetLightingSettings() { return m_lighting; }
	SceneDataCB& getCBData() { return m_sceneDataCB; }
	SkyboxSettings& getSkyboxSettings() { return m_skybox; }
	const SkyboxSettings& getSkyboxSettings() const { return m_skybox; }

	bool applySkyboxToRenderer();

	Quadtree& getQuadtree() { return *m_quadtree; }
private:
	std::string m_name = "SampleScene";

	std::vector<GameObject*>	m_gameObjects;
	SceneLightingSettings		m_lighting;
	Quadtree*					m_quadtree;
	SceneDataCB					m_sceneDataCB;
	SkyboxSettings				m_skybox;

};