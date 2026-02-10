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
	bool init()	override;
	void update() override;
	void preRender() override;

	void CreateGameObject();
	void AddGameObject(GameObject* gameObject);
	void DetachGameObject(GameObject* gameObject);
	void DestroyGameObject(GameObject* gameObject);

	void getGameObjectToRender(std::vector<GameObject*>& renderableGameObjects);
	const std::vector<GameObject*>& getAllGameObjects() { return m_gameObjects; }

	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix);

	const char* getName() { return (char*)m_name.c_str(); }
	SceneData&	getData() { return m_sceneData; }

	Quadtree&	getQuadtree() { return *m_quadtree; }
private:
	std::string m_name = "SampleScene";
	short m_current_uuid = 0;

	std::vector<GameObject*>	m_gameObjects;
	SceneData					m_sceneData;
	Quadtree*					m_quadtree;
};

