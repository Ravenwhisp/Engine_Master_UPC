#pragma once
#include "GameObject.h"

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

namespace Emeika {
	class Scene
	{
	public:
		Scene();
		~Scene();

		void add(GameObject* gameObject);
		void remove(GameObject* gameObject);

		void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix);

		SceneData&					getData();
		const char*					getName() { return (char*) m_name.c_str(); }
		std::vector<GameObject*>	getGameObjects() { return m_gameObjects; }

	private:
		std::string m_name = "SampleScene";
		std::vector<GameObject*> m_gameObjects;
		SceneData m_sceneData;
	};
}


