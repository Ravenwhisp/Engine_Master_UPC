#pragma once

#include "SceneLightingSettings.h"
#include "SceneDataCB.h"
#include "SkyBoxSettings.h"

class Quadtree;
class GameObject;
class CameraComponent;
class MeshRenderer;

struct SceneSnapshot;

class Scene
{
private:
	std::string m_name = "SampleScene";

	std::vector<std::unique_ptr<GameObject>>	m_allObjects;
	std::vector<GameObject*>					m_rootObjects;
	std::vector<MeshRenderer*>					m_meshRenderers;

	std::unique_ptr<Quadtree>			m_quadtree;

	SceneLightingSettings		m_lighting;
	SceneDataCB					m_sceneDataCB;
	SkyBoxSettings				m_skybox;

	CameraComponent* m_defaultCamera = nullptr;

public:
	Scene();
	~Scene();

#pragma region GameLoop
	bool init();
	void update();
	void render(ID3D12GraphicsCommandList* commandList);
	bool cleanUp();
#pragma endregion

	const char* getName() { return (char*)m_name.c_str(); }
	const void setName(const char* newName) { m_name = newName; }

	SceneLightingSettings& GetLightingSettings() { return m_lighting; }
	SceneDataCB& getCBData() { return m_sceneDataCB; }
	SkyBoxSettings& getSkyBoxSettings() { return m_skybox; }
	const SkyBoxSettings& getSkyBoxSettings() const { return m_skybox; }

	bool applySkyBoxToRenderer();

	Quadtree* getQuadtree() { return m_quadtree.get(); }
	void createQuadtree();
	void resetQuadtree();

	CameraComponent* getDefaultCamera() const { return m_defaultCamera; }
	void setDefaultCamera(CameraComponent* camera) { m_defaultCamera = camera; }

	const std::vector<MeshRenderer*>& getAllMeshRenderers() { return m_meshRenderers; }

	void createGameObject();
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

	void clearScene();
};