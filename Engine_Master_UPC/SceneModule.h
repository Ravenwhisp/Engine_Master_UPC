#pragma once
#include "Module.h"
#include "GameObject.h"
#include "Quadtree.h"

struct SceneData {
	Vector3 lightDirection;
	float pad0;
	Vector3 lightColor;
	float pad1;
	Vector3 ambientColor;
	float pad2;
	Vector3 view;
	float pad3;
};

class SceneModule : public Module
{
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

	void createGameObject();
	void removeGameObject(const int uuid);

	void addGameObject(GameObject* gameObject);
	void detachGameObject(GameObject* gameObject);
	void destroyGameObject(GameObject* gameObject);

	GameObject* findInHierarchy(GameObject* current, int uuid);
	void destroyHierarchy(GameObject* obj);

	const std::vector<GameObject*>& getAllGameObjects() { return m_gameObjects; }

	const char* getName() { return (char*)m_name.c_str(); }
	SceneData& getData() { return m_sceneData; }

	Quadtree& getQuadtree() { return *m_quadtree; }
private:
	std::string m_name = "SampleScene";

	std::vector<GameObject*>	m_gameObjects;
	SceneData					m_sceneData;
	Quadtree*					m_quadtree;
};