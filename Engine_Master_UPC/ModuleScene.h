#pragma once
#include "Module.h"
#include <rapidjson/document.h>
#include "MD5Fwd.h"
#include "MeshRenderer.h"
#include "SceneSnapshot.h"

class SceneSerializer;
class GameObject;
class Quadtree;
class CameraComponent;

struct SceneDataCB
{
	Vector3 viewPos;
	float pad0 = 0.0f;
};

struct SceneLightingSettings
{
	Vector3 ambientColor;
	float ambientIntensity;
};

struct SkyBoxSettings
{
	bool enabled = true;
	MD5Hash cubemapAssetId = INVALID_ASSET_ID;
};

class ModuleScene : public Module
{
private:
	std::string m_name = "SampleScene";

	std::vector<std::unique_ptr<GameObject>>	m_allObjects;
	std::vector<GameObject*>					m_rootObjects;
	std::vector<MeshRenderer*>					m_meshRenderers;

	std::unique_ptr<SceneSerializer>	m_sceneSerializer;
	std::unique_ptr<Quadtree>			m_quadtree;

	SceneLightingSettings		m_lighting;
	SceneDataCB					m_sceneDataCB;
	SkyBoxSettings				m_skybox;

	CameraComponent* m_defaultCamera = nullptr;
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
	void clearScene();
#pragma endregion

	GameObject* createGameObject();
	GameObject* createGameObjectWithUID(UID id, UID transformUID);
	GameObject* findGameObjectByUID(UID uuid);
	void removeGameObject(const UID uuid);

	void addGameObject(std::unique_ptr<GameObject> gameObject);
	void destroyGameObject(GameObject* gameObject);
	void resetGameObjects(SceneSnapshot previousScene);

	GameObject* findInWindowHierarchy(GameObject* current, UID uuid);
	void destroyWindowHierarchy(GameObject* obj);

	void addToRootList(GameObject* gameObject);
	void removeFromRootList(GameObject* gameObject);
	const std::vector<GameObject*>& getRootObjects() const;

	GameObject* createDirectionalLightOnInit();

	std::vector<GameObject*> getAllGameObjects();

	SceneSnapshot getClonedGameObjects();
	std::unique_ptr<GameObject> cloneGameObjectRecursive(GameObject* original, SceneSnapshot& result);
	void fixClonedReferences(const SceneSnapshot& snapshot);

	const std::vector<MeshRenderer*>& getAllMeshRenderers() { return m_meshRenderers; }

	const char* getName() { return (char*)m_name.c_str(); }
	const void setName(const char* newName) { m_name = newName; }

	SceneLightingSettings& GetLightingSettings() { return m_lighting; }
	SceneDataCB& getCBData() { return m_sceneDataCB; }
	SkyBoxSettings& getSkyBoxSettings() { return m_skybox; }
	const SkyBoxSettings& getSkyBoxSettings() const { return m_skybox; }

	bool applySkyBoxToRenderer();

	Quadtree* getQuadtree() { return m_quadtree.get(); }
	void createQuadtree();

	CameraComponent* getDefaultCamera() const { return m_defaultCamera; }
	void setDefaultCamera(CameraComponent* camera) { m_defaultCamera = camera; }

	bool initEmpty();
};